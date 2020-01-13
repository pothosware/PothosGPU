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
    const std::string& device,
    const AfArrayScalarOp<T>& func,
    const Pothos::DType& dtype,
    T scalar,
    size_t numChans,
    bool allowZeroOperand
): OneToOneBlock(
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
    static const T Zero(0);

    if(!_allowZeroOperand && (scalar == Zero))
    {
        throw Pothos::InvalidArgumentException("Scalar cannot be zero.");
    }

    _scalar = PothosToAF<T>::to(scalar);
    _func.bind(_scalar, 1);

    this->emitSignal("scalarChanged", scalar);
}

template <typename T>
void ScalarOpBlock<T>::work()
{
    if(0 == this->workInfo().minElements)
    {
        return;
    }

    auto afInput = this->getInputsAsAfArray();
    _func.bind(afInput, 0);

    OneToOneBlock::work(afInput);
}

// ArrayFire does not support std::int8_t.
template class ScalarOpBlock<std::int16_t>;
template class ScalarOpBlock<std::int32_t>;
template class ScalarOpBlock<std::int64_t>;
template class ScalarOpBlock<std::uint8_t>;
template class ScalarOpBlock<std::uint16_t>;
template class ScalarOpBlock<std::uint32_t>;
template class ScalarOpBlock<std::uint64_t>;
template class ScalarOpBlock<float>;
template class ScalarOpBlock<double>;
// ArrayFire does not support any integral complex type.
template class ScalarOpBlock<std::complex<float>>;
template class ScalarOpBlock<std::complex<double>>;
