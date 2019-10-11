// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <Pothos/Framework.hpp>
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

static void testDTypeEnumUsage(
    const std::string& dtypeName,
    ::af_dtype afDType)
{
    Pothos::DType dtype(dtypeName);
    POTHOS_TEST_EQUAL(
        afDType,
        Pothos::Object(dtype.name()).convert<::af_dtype>());

    Pothos::DType dtypeFromAF(Pothos::Object(afDType).convert<std::string>());
    POTHOS_TEST_EQUAL(dtypeName, dtypeFromAF.name());
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_dtype_conversion)
{
    testTypesCanConvert<::af_dtype>();
    testDTypeEnumUsage("int16",           ::s16);
    testDTypeEnumUsage("int32",           ::s32);
    testDTypeEnumUsage("int64",           ::s64);
    testDTypeEnumUsage("uint8",           ::u8);
    testDTypeEnumUsage("uint16",          ::u16);
    testDTypeEnumUsage("uint32",          ::u32);
    testDTypeEnumUsage("uint64",          ::u64);
    testDTypeEnumUsage("float32",         ::f32);
    testDTypeEnumUsage("float64",         ::f64);
    testDTypeEnumUsage("complex_float32", ::c32);
    testDTypeEnumUsage("complex_float64", ::c64);
}
