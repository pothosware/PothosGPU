// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ScalarOpBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cassert>
#include <cstring>
#include <string>
#include <typeinfo>

template <typename T>
ScalarOpBlock<T>::ScalarOpBlock(
    const AfArrayScalarOp<T>& func,
    const Pothos::DType& dtype,
    T scalar,
    size_t numChans
): OneToOneBlock(
       Pothos::Callable(func),
       dtype,
       dtype,
       numChans)
{
    setScalar(scalar);

    this->registerCall(this, POTHOS_FCN_TUPLE(Class, getScalar));
    this->registerCall(this, POTHOS_FCN_TUPLE(Class, setScalar));
}

template <typename T>
ScalarOpBlock<T>::~ScalarOpBlock() {};

template <typename T>
T ScalarOpBlock<T>::getScalar() const
{
    return PothosToAF<T>::from(_scalar);
}

template <typename T>
void ScalarOpBlock<T>::setScalar(const T& scalar)
{
    _scalar = PothosToAF<T>::to(scalar);
    _func.bind(_scalar, 1);
}

template <typename T>
void ScalarOpBlock<T>::work()
{
    auto afInput = this->getInputsAsAfArray();
    _func.bind(afInput, 0);

    OneToOneBlock::work(afInput);
}

//
// Since the operators are so overloaded, we need to enter macro hell to
// do all of this.
//

#define ifTypeDeclareFactory(T, op) \
    if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        return new ScalarOpBlock<T>(&af::operator op, dtype, T(0), numChans);

#define ScalarOpBlock(opName, op) \
    static Pothos::Block* scalarFactory_ ## opName ( \
        const Pothos::DType& dtype, \
        size_t numChans) \
    { \
        ifTypeDeclareFactory(std::int16_t, op) \
        ifTypeDeclareFactory(std::int32_t, op) \
        ifTypeDeclareFactory(std::int64_t, op) \
        ifTypeDeclareFactory(std::uint8_t, op) \
        ifTypeDeclareFactory(std::uint16_t, op) \
        ifTypeDeclareFactory(std::uint32_t, op) \
        ifTypeDeclareFactory(std::uint64_t, op) \
        ifTypeDeclareFactory(float, op) \
        ifTypeDeclareFactory(double, op) \
        ifTypeDeclareFactory(std::complex<float>, op) \
        ifTypeDeclareFactory(std::complex<double>, op) \
     \
        throw Pothos::InvalidArgumentException( \
                  "Unsupported type", \
                  dtype.name()); \
    } \
    static Pothos::BlockRegistry register_ ## opName ( \
        "/arrayfire/scalar/" #opName, \
        Pothos::Callable(scalarFactory_ ## opName));

// TODO: shift left,right, scalar is integral

ScalarOpBlock(add, +)
ScalarOpBlock(sub, -)
ScalarOpBlock(mult, *)
ScalarOpBlock(div, /)
ScalarOpBlock(bitwise_or, |)
ScalarOpBlock(bitwise_and, &)
ScalarOpBlock(bitwise_xor, ^)
