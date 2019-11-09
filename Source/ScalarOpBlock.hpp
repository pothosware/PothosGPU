// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>

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
            const AfArrayScalarOp<T>& func,
            const Pothos::DType& dtype,
            T scalar,
            size_t numChans);

        virtual ~ScalarOpBlock();

        T getScalar() const;

        void setScalar(const T& scalar);

        void work() override;

    private:
        typename PothosToAF<T>::type _scalar;
};

//
// Since the operators are so overloaded, we need to enter macro hell to
// do all of this.
//

#define ScalarOpIfTypeDeclareFactory(T, op) \
    if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        return new ScalarOpBlock<T>(&af::operator op, dtype, T(0), numChans);

#define ScalarOpBlockFactory(opName, op) \
    static Pothos::Block* scalarFactory_ ## opName ( \
        const Pothos::DType& dtype, \
        size_t numChans) \
    { \
        ScalarOpIfTypeDeclareFactory(std::int16_t, op) \
        ScalarOpIfTypeDeclareFactory(std::int32_t, op) \
        ScalarOpIfTypeDeclareFactory(std::int64_t, op) \
        ScalarOpIfTypeDeclareFactory(std::uint8_t, op) \
        ScalarOpIfTypeDeclareFactory(std::uint16_t, op) \
        ScalarOpIfTypeDeclareFactory(std::uint32_t, op) \
        ScalarOpIfTypeDeclareFactory(std::uint64_t, op) \
        ScalarOpIfTypeDeclareFactory(float, op) \
        ScalarOpIfTypeDeclareFactory(double, op) \
        ScalarOpIfTypeDeclareFactory(std::complex<float>, op) \
        ScalarOpIfTypeDeclareFactory(std::complex<double>, op) \
     \
        throw Pothos::InvalidArgumentException( \
                  "Unsupported type", \
                  dtype.name()); \
    } \
    static Pothos::BlockRegistry register_ ## opName ( \
        "/arrayfire/scalar/" #opName, \
        Pothos::Callable(scalarFactory_ ## opName));
