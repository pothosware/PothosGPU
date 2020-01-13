// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

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
            T scalar,
            size_t numChans,
            bool allowZeroOperand);

        virtual ~ScalarOpBlock();

        T getScalar() const;

        void setScalar(const T& scalar);

        void work() override;

    private:
        typename PothosToAF<T>::type _scalar;

        bool _allowZeroOperand;
};

//
// Since the operators are so overloaded, we need to enter macro hell to
// do all of this.
//

#define ScalarOpIfTypeDeclareFactory(T, op, allowZeroOperand) \
    if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        return new ScalarOpBlock<T>( \
                       device, \
                       &af::operator op, \
                       dtype, \
                       scalarObject.convert<T>(), \
                       numChans, \
                       allowZeroOperand);

#define ScalarOpBlockFactory(opName, op, allowZeroOperand, intOnly) \
    static Pothos::Block* scalarFactory_ ## opName ( \
        const std::string& device, \
        const Pothos::DType& dtype, \
        Pothos::Object scalarObject, \
        size_t numChans) \
    { \
        ScalarOpIfTypeDeclareFactory(std::int16_t, op, allowZeroOperand) \
        ScalarOpIfTypeDeclareFactory(std::int32_t, op, allowZeroOperand) \
        ScalarOpIfTypeDeclareFactory(std::int64_t, op, allowZeroOperand) \
        ScalarOpIfTypeDeclareFactory(std::uint8_t, op, allowZeroOperand) \
        ScalarOpIfTypeDeclareFactory(std::uint16_t, op, allowZeroOperand) \
        ScalarOpIfTypeDeclareFactory(std::uint32_t, op, allowZeroOperand) \
        ScalarOpIfTypeDeclareFactory(std::uint64_t, op, allowZeroOperand) \
        if(!intOnly) \
        { \
            ScalarOpIfTypeDeclareFactory(float, op, allowZeroOperand) \
            ScalarOpIfTypeDeclareFactory(double, op, allowZeroOperand) \
            ScalarOpIfTypeDeclareFactory(std::complex<float>, op, allowZeroOperand) \
            ScalarOpIfTypeDeclareFactory(std::complex<double>, op, allowZeroOperand) \
        } \
     \
        throw Pothos::InvalidArgumentException( \
                  "Unsupported type", \
                  dtype.name()); \
    } \
    static Pothos::BlockRegistry register_ ## opName ( \
        "/arrayfire/scalar/" #opName, \
        Pothos::Callable(scalarFactory_ ## opName));
