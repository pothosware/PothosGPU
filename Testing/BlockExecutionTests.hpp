// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "TestUtility.hpp"

#include <complex>
#include <cmath>
#include <functional>

template <typename T>
using UnaryFunc = std::function<T(const T&)>;

template <typename T>
void testOneToOneBlock(
    const std::string& blockRegistryPath,
    size_t numChannels,
    const UnaryFunc<T>& verificationFunc);

template <typename T>
static inline EnableIfFloat<T, T> testSigmoid(const T& val)
{
    return (T(1.0) / (1.0 + std::exp(val * T(-1))));
}
