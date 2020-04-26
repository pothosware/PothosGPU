// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "TestUtility.hpp"

namespace AFTests
{

//
// Templated type calls
//

template <typename T>
void testOneToOneBlock(const std::string& blockRegistryPath);

template <typename T>
void testOneToOneBlockF2C(const std::string& blockRegistryPath);

template <typename T>
void testOneToOneBlockC2F(const std::string& blockRegistryPath);

template <typename T>
void testTwoToOneBlock(
    const std::string& blockRegistryPath,
    bool removeZerosInBuffer1);

template <typename T>
void testTwoToOneBlockF2C(
    const std::string& blockRegistryPath,
    bool removeZerosInBuffer1);

template <typename T>
void testNToOneBlock(
    const std::string& blockRegistryPath,
    size_t numChannels);

template <typename T1, typename T2>
void testReducedBlock(
    const std::string& blockRegistryPath,
    size_t numChannels);

template <typename T>
void testScalarOpBlock(
    const std::string& blockRegistryPath,
    size_t numChannels,
    bool allowZeroScalar);

}
