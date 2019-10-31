// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>

#include <complex>
#include <cmath>
#include <functional>

using PortInfoVector = std::vector<Pothos::PortInfo>;

template <typename In, typename Out>
using UnaryFunc = std::function<Out(const In&)>;

template <typename T>
void testOneToOneBlock(
    const std::string& blockRegistryPath,
    size_t numChannels,
    const UnaryFunc<T, T>& verificationFunc);

template <typename In, typename Out>
void testOneToOneBlock(
    const std::string& blockRegistryPath,
    size_t numChannels,
    const UnaryFunc<In, Out>& verificationFunc);

//
// Manual verification functions
//

template <typename T>
static inline EnableIfComplex<T, typename T::value_type> testReal(const T& val)
{
    return val.real();
}

template <typename T>
static inline EnableIfComplex<T, typename T::value_type> testImag(const T& val)
{
    return val.imag();
}

template <typename T>
static inline EnableIfFloat<T, T> testSigmoid(const T& val)
{
    return (T(1.0) / (1.0 + std::exp(val * T(-1))));
}

template <typename T>
static inline EnableIfFloat<T, T> testFactorial(const T& val)
{
    return std::tgamma(val + T(1.0));
}
