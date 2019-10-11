// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <string>
#include <typeinfo>

template <typename EnumType>
static void testTypesCanConvert()
{
    POTHOS_TEST_TRUE(Pothos::Object::canConvert(
                         typeid(std::string),
                         typeid(EnumType)));
    POTHOS_TEST_TRUE(Pothos::Object::canConvert(
                         typeid(EnumType),
                         typeid(std::string)));
}

template <typename EnumType>
static void testEnumValueConversion(
    const std::string& stringVal,
    EnumType enumVal)
{
    POTHOS_TEST_EQUAL(
        enumVal,
        Pothos::Object(stringVal).convert<EnumType>());
    POTHOS_TEST_EQUAL(
        stringVal,
        Pothos::Object(enumVal).convert<std::string>());
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_backend_conversion)
{
    testTypesCanConvert<::af_backend>();
    testEnumValueConversion("CPU",    ::AF_BACKEND_CPU);
    testEnumValueConversion("CUDA",   ::AF_BACKEND_CUDA);
    testEnumValueConversion("OpenCL", ::AF_BACKEND_OPENCL);
}
