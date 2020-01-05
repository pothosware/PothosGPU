// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <string>
#include <typeinfo>

template <typename Type1, typename Type2>
static void testTypesCanConvert()
{
    POTHOS_TEST_TRUE(Pothos::Object::canConvert(
                         typeid(Type1),
                         typeid(Type2)));
    POTHOS_TEST_TRUE(Pothos::Object::canConvert(
                         typeid(Type2),
                         typeid(Type1)));
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
    testTypesCanConvert<std::string, af::Backend>();
    testEnumValueConversion("CPU",    ::AF_BACKEND_CPU);
    testEnumValueConversion("CUDA",   ::AF_BACKEND_CUDA);
    testEnumValueConversion("OpenCL", ::AF_BACKEND_OPENCL);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_convmode_conversion)
{
    testTypesCanConvert<std::string, af::convMode>();
    testEnumValueConversion("Default", ::AF_CONV_DEFAULT);
    testEnumValueConversion("Expand",  ::AF_CONV_EXPAND);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_convdomain_conversion)
{
    testTypesCanConvert<std::string, af::convDomain>();
    testEnumValueConversion("Auto",    ::AF_CONV_AUTO);
    testEnumValueConversion("Spatial", ::AF_CONV_SPATIAL);
    testEnumValueConversion("Freq",    ::AF_CONV_FREQ);
}

#if AF_API_VERSION_CURRENT >= 34
POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_randomenginetype_conversion)
{
    testTypesCanConvert<std::string, af::randomEngineType>();
    testEnumValueConversion("Philox",   ::AF_RANDOM_ENGINE_PHILOX);
    testEnumValueConversion("Threefry", ::AF_RANDOM_ENGINE_THREEFRY);
    testEnumValueConversion("Mersenne", ::AF_RANDOM_ENGINE_MERSENNE);
}
#endif

static void testDTypeEnumUsage(
    const std::string& dtypeName,
    af::dtype afDType)
{
    Pothos::DType dtype(dtypeName);
    POTHOS_TEST_EQUAL(
        afDType,
        Pothos::Object(dtype).convert<af::dtype>());

    auto dtypeFromAF = Pothos::Object(afDType).convert<Pothos::DType>();
    POTHOS_TEST_EQUAL(dtypeName, dtypeFromAF.name());
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_dtype_conversion)
{
    testTypesCanConvert<Pothos::DType, af::dtype>();
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
