// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <complex>
#include <cstdint>
#include <functional>

template <typename T>
using UnaryFunc = std::function<T(const T&)>;

template <typename T>
void testOneToOneBlock(
    const std::string& blockRegistryPath,
    size_t numChannels,
    const UnaryFunc<T>& verificationFunc);
