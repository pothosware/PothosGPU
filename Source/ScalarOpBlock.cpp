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
            const Pothos::DType& outputDType,
            const T& scalar,
            bool allowZeroOperand
        ):
        OneToOneBlock(
            device,
            Pothos::Callable(func),
            dtype,
            outputDType),
        _allowZeroOperand(allowZeroOperand)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, scalar));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setScalar));

            this->registerProbe("scalar");
            this->registerSignal("scalarChanged");

            setScalar(scalar);
        }

        virtual ~ScalarOpBlock() = default;

        T scalar() const
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

    private:
        typename PothosToAF<T>::type _scalar;

        bool _allowZeroOperand;
};

//
// Factories
//
// These blocks have the same implementation but make sense to separate.
// TODO: better error messages
//

enum class ScalarBlockType
{
    ARITHMETIC,
    COMPARATOR,
    BITWISE,
    LOGICAL
};

#define IfTypeThenLambda(op, opStr, callerOp, cType, dest) \
    if(opStr == callerOp) \
        dest = [](const af::array& a, const PothosToAF<cType>::type& b){return (a op b);};

static Pothos::Block* makeScalarOpBlock(
    ScalarBlockType blockType,
    const std::string& device,
    const std::string& operation,
    const Pothos::DType& dtype,
    const Pothos::Object& scalarObject)
{
    const bool allowZeroScalar = ("/" != operation) && ("%" != operation);
    static const Pothos::DType Int8DType("int8");

    #define IfTypeDeclareFactory(cType) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(cType))) \
        { \
            AfArrayScalarOp<cType> func = nullptr; \
            switch(blockType) \
            { \
                case ScalarBlockType::ARITHMETIC: \
                    IfTypeThenLambda(+, "ADD", operation, cType, func) \
                    else IfTypeThenLambda(-, "SUBTRACT", operation, cType, func) \
                    else IfTypeThenLambda(*, "MULTIPLY", operation, cType, func) \
                    else IfTypeThenLambda(/, "DIVIDE", operation, cType, func) \
                    else IfTypeThenLambda(%, "MODULUS", operation, cType, func) \
                    else throw Pothos::InvalidArgumentException("Invalid operation", operation); \
                    break; \
                case ScalarBlockType::COMPARATOR: \
                    IfTypeThenLambda(>, ">", operation, cType, func) \
                    else IfTypeThenLambda(>=, ">=", operation, cType, func) \
                    else IfTypeThenLambda(<, "<", operation, cType, func) \
                    else IfTypeThenLambda(<=, "<=", operation, cType, func) \
                    else IfTypeThenLambda(==, "==", operation, cType, func) \
                    else IfTypeThenLambda(!=, "!=", operation, cType, func) \
                    else throw Pothos::InvalidArgumentException("Invalid operation", operation); \
                    break; \
                case ScalarBlockType::BITWISE: \
                    IfTypeThenLambda(&, "AND", operation, cType, func) \
                    else IfTypeThenLambda(|, "OR", operation, cType, func) \
                    else IfTypeThenLambda(^, "XOR", operation, cType, func) \
                    else IfTypeThenLambda(<<, "LEFTSHIFT", operation, cType, func) \
                    else IfTypeThenLambda(>>, "RIGHTSHIFT", operation, cType, func) \
                    else throw Pothos::InvalidArgumentException("Invalid operation", operation); \
                    break; \
                case ScalarBlockType::LOGICAL: \
                    IfTypeThenLambda(&&, "AND", operation, cType, func) \
                    else IfTypeThenLambda(||, "OR", operation, cType, func) \
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
                           ((ScalarBlockType::COMPARATOR == blockType) || (ScalarBlockType::LOGICAL == blockType)) ? Int8DType : dtype, \
                           scalarObject.convert<cType>(), \
                           allowZeroScalar); \
        }

    IfTypeDeclareFactory(std::int16_t)
    IfTypeDeclareFactory(std::int32_t)
    IfTypeDeclareFactory(std::int64_t)
    IfTypeDeclareFactory(std::uint8_t)
    IfTypeDeclareFactory(std::uint16_t)
    IfTypeDeclareFactory(std::uint32_t)
    IfTypeDeclareFactory(std::uint64_t)

    if((ScalarBlockType::ARITHMETIC == blockType) || (ScalarBlockType::COMPARATOR == blockType))
    {
        IfTypeDeclareFactory(float)
        IfTypeDeclareFactory(double)
    }
    if(ScalarBlockType::ARITHMETIC == blockType)
    {
        if("MODULUS" != operation)
        {
            IfTypeDeclareFactory(std::complex<float>)
            IfTypeDeclareFactory(std::complex<double>)
        }
        else
        {
            throw Pothos::InvalidArgumentException("Invalid operation for type", operation);
        }
    }

    throw Pothos::InvalidArgumentException("Invalid type", dtype.name());
}

//
// Block registries
//

/*
 * |PothosDoc Scalar Arithmetic
 *
 * Perform the specified arithmetic operation on the input stream, using
 * the specified scalar value as the second operand. The result is returned
 * in the output stream.
 *
 * |category /ArrayFire/Scalar Operations
 * |keywords scalar add subtract multiply divide modulus
 * |factory /arrayfire/scalar/arithmetic(device,operation,dtype,scalar)
 * |setter setScalar(scalar)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param operation[Operation] The arithmetic operation to perform.
 * |widget ComboBox(editable=false)
 * |option [Add] "ADD"
 * |option [Subtract] "SUBTRACT"
 * |option [Multiply] "MULTIPLY"
 * |option [Divide] "DIVIDE"
 * |option [Modulus] "MODULUS"
 * |default "ADD"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1)
 * |default "float64"
 * |preview disable
 *
 * |param scalar[Scalar] The scalar value used in the operation.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 */
static Pothos::BlockRegistry registerScalarArithmetic(
    "/arrayfire/scalar/arithmetic",
    Pothos::Callable(&makeScalarOpBlock).bind(ScalarBlockType::ARITHMETIC, 0));

/*
 * |PothosDoc Scalar Comparator
 *
 * Perform the specified comparison between each element in the input stream
 * and the given scalar, returning the result in an output stream of type <b>int8</b>,
 * where <b>0</b> corresponds to false and <b>1</b> corresponds to true.
 *
 * |category /ArrayFire/Scalar Operations
 * |keywords scalar less greater not equal
 * |factory /arrayfire/scalar/comparator(device,operation,dtype,scalar)
 * |setter setScalar(scalar)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param operation[Operation] The comparison to perform.
 * |widget ComboBox(editable=false)
 * |option [>] ">"
 * |option [>=] ">="
 * |option [<] "<"
 * |option [<=] "<="
 * |option [==] "=="
 * |option [!=] "!="
 * |default ">"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1)
 * |default "float64"
 * |preview disable
 *
 * |param scalar[Scalar] The scalar value used in the operation.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 */
static Pothos::BlockRegistry registerScalarComparator(
    "/arrayfire/scalar/comparator",
    Pothos::Callable(&makeScalarOpBlock).bind(ScalarBlockType::COMPARATOR, 0));

/*
 * |PothosDoc Scalar Bitwise
 *
 * Perform the specified bitwise operation between each element in the input stream
 * and the given scalar, returning the result in an output stream.
 *
 * |category /ArrayFire/Scalar Operations
 * |keywords scalar and or left right shift
 * |factory /arrayfire/scalar/bitwise(device,operation,dtype,scalar)
 * |setter setScalar(scalar)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param operation[Operation] The bitwise operation to perform.
 * |widget ComboBox(editable=false)
 * |option [And] "AND"
 * |option [Or] "OR"
 * |option [Left Shift] "LEFTSHIFT"
 * |option [Right Shift] "RIGHTSHIFT"
 * |default "AND"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1)
 * |default "uint64"
 * |preview disable
 *
 * |param scalar[Scalar] The scalar value used in the operation.
 * |widget SpinBox()
 * |default 0
 * |preview enable
 */
static Pothos::BlockRegistry registerScalarBitwise(
    "/arrayfire/scalar/bitwise",
    Pothos::Callable(&makeScalarOpBlock).bind(ScalarBlockType::BITWISE, 0));

/*
 * |PothosDoc Scalar Logical
 *
 * Perform the specified logical operation between each element in the input stream
 * and the given scalar, returning the result in an output stream of type <b>int8</b>,
 * where <b>0</b> corresponds to false and <b>1</b> corresponds to true.
 *
 * |category /ArrayFire/Scalar Operations
 * |keywords scalar and or
 * |factory /arrayfire/scalar/logical(device,operation,dtype,scalar)
 * |setter setScalar(scalar)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param operation[Operation] The bitwise operation to perform.
 * |widget ComboBox(editable=false)
 * |option [And] "AND"
 * |option [Or] "OR"
 * |default "AND"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1)
 * |default "uint64"
 * |preview disable
 *
 * |param scalar[Scalar] The scalar value used in the operation.
 * |widget SpinBox()
 * |default 0
 * |preview enable
 */
static Pothos::BlockRegistry registerScalarLogical(
    "/arrayfire/scalar/logical",
    Pothos::Callable(&makeScalarOpBlock).bind(ScalarBlockType::LOGICAL, 0));
