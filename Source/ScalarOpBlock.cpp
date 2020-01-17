// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cassert>
#include <cstring>
#include <string>
#include <typeinfo>

template <typename T>
using AfArrayScalarOp = af::array(*)(
                            const af::array&,
                            const typename PothosToAF<T>::type&);

template <typename T>
class ScalarOpBlock: public OneToOneBlock
{
    public:

        using Class = ScalarOpBlock<T>;

        ScalarOpBlock(
            const std::string& device,
            const AfArrayScalarOp<T>& func,
            const Pothos::DType& dtype,
            const T& scalar,
            size_t numChans,
            bool allowZeroOperand
        ):
        OneToOneBlock(
            device,
            Pothos::Callable(func),
            dtype,
            dtype,
            numChans),
        _allowZeroOperand(allowZeroOperand)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getScalar));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setScalar));

            this->registerProbe("getScalar", "scalarChanged", "setScalar");

            setScalar(scalar);
        }

        virtual ~ScalarOpBlock() = default;

        T getScalar() const
        {
            return PothosToAF<T>::from(_scalar);
        }

        void setScalar(const T& scalar)
        {
            static const T Zero(0);

            if(!_allowZeroOperand && (scalar == Zero))
            {
                throw Pothos::InvalidArgumentException("Scalar cannot be zero.");
            }

            _scalar = PothosToAF<T>::to(scalar);
            _func.bind(_scalar, 1);

            this->emitSignal("scalarChanged", scalar);
        }

        void work() override
        {
            if(0 == this->workInfo().minElements)
            {
                return;
            }

            auto afInput = this->getInputsAsAfArray();
            _func.bind(afInput, 0);

            OneToOneBlock::work(afInput);
        }

    private:
        typename PothosToAF<T>::type _scalar;

        bool _allowZeroOperand;
};

//
// Factories
//
// These blocks have the same implementation but make sense to separate.
//

enum class ScalarBlockType
{
    ARITHMETIC,
    COMPARATOR,
    BITWISE,
    LOGICAL
};

#define IfTypeThenLambda(op, callerOp, cType, dest) \
    if(#op == callerOp) \
        dest = [](const af::array& a, const PothosToAF<cType>::type& b){return a op b;};

static Pothos::Block* makeScalarOpBlock(
    ScalarBlockType blockType,
    const std::string& device,
    const std::string& operation,
    const Pothos::DType& dtype,
    const Pothos::Object& scalarObject,
    size_t numChans)
{
    const bool allowZeroScalar = ("/" != operation) && ("%" != operation);

    #define IfTypeDeclareFactory(cType) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(cType))) \
        { \
            AfArrayScalarOp<cType> func = nullptr; \
            switch(blockType) \
            { \
                case ScalarBlockType::ARITHMETIC: \
                    IfTypeThenLambda(+, operation, cType, func) \
                    else IfTypeThenLambda(-, operation, cType, func) \
                    else IfTypeThenLambda(*, operation, cType, func) \
                    else IfTypeThenLambda(/, operation, cType, func) \
                    else IfTypeThenLambda(%, operation, cType, func) \
                    else throw Pothos::InvalidArgumentException("Invalid operation", operation); \
                    break; \
                case ScalarBlockType::COMPARATOR: \
                    IfTypeThenLambda(>, operation, cType, func) \
                    else IfTypeThenLambda(>=, operation, cType, func) \
                    else IfTypeThenLambda(<, operation, cType, func) \
                    else IfTypeThenLambda(<=, operation, cType, func) \
                    else IfTypeThenLambda(==, operation, cType, func) \
                    else IfTypeThenLambda(!=, operation, cType, func) \
                    else throw Pothos::InvalidArgumentException("Invalid operation", operation); \
                    break; \
                case ScalarBlockType::BITWISE: \
                    IfTypeThenLambda(&, operation, cType, func) \
                    else IfTypeThenLambda(|, operation, cType, func) \
                    else IfTypeThenLambda(^, operation, cType, func) \
                    else IfTypeThenLambda(<<, operation, cType, func) \
                    else IfTypeThenLambda(>>, operation, cType, func) \
                    else throw Pothos::InvalidArgumentException("Invalid operation", operation); \
                    break; \
                case ScalarBlockType::LOGICAL: \
                    IfTypeThenLambda(&&, operation, cType, func) \
                    else IfTypeThenLambda(||, operation, cType, func) \
                    else throw Pothos::InvalidArgumentException("Invalid operation", operation); \
                    break; \
                default: \
                    throw Pothos::AssertionViolationException("Invalid ScalarBlockType"); \
            } \
            if(nullptr == func) throw Pothos::AssertionViolationException("nullptr == func"); \
 \
            return new ScalarOpBlock<cType>( \
                           device, \
                           func, \
                           dtype, \
                           scalarObject.convert<cType>(), \
                           numChans, \
                           allowZeroScalar); \
        }

    IfTypeDeclareFactory(std::int16_t)
    IfTypeDeclareFactory(std::int32_t)
    IfTypeDeclareFactory(std::int64_t)
    IfTypeDeclareFactory(std::uint8_t)
    IfTypeDeclareFactory(std::uint16_t)
    IfTypeDeclareFactory(std::uint32_t)
    IfTypeDeclareFactory(std::uint64_t)
    IfTypeDeclareFactory(float)
    IfTypeDeclareFactory(double)
    IfTypeDeclareFactory(std::complex<float>)
    IfTypeDeclareFactory(std::complex<double>)

    throw Pothos::InvalidArgumentException("Invalid type", dtype.name());
}

static Pothos::BlockRegistry registerScalarArithmetic(
    "/arrayfire/scalar/arithmetic",
    Pothos::Callable(&makeScalarOpBlock).bind(ScalarBlockType::ARITHMETIC, 0));

static Pothos::BlockRegistry registerScalarComparator(
    "/arrayfire/scalar/comparator",
    Pothos::Callable(&makeScalarOpBlock).bind(ScalarBlockType::COMPARATOR, 0));

static Pothos::BlockRegistry registerScalarBitwise(
    "/arrayfire/scalar/bitwise",
    Pothos::Callable(&makeScalarOpBlock).bind(ScalarBlockType::BITWISE, 0));

static Pothos::BlockRegistry registerScalarLogical(
    "/arrayfire/scalar/logical",
    Pothos::Callable(&makeScalarOpBlock).bind(ScalarBlockType::LOGICAL, 0));
