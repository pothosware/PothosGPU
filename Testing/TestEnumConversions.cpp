// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <string>
#include <typeinfo>

namespace AFTests
{

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

static void testDTypeEnumUsage(
    const std::string& dtypeName,
    af::dtype afDType)
{
    Pothos::DType dtype(dtypeName);
    POTHOS_TEST_EQUAL(
        afDType,
        Pothos::Object(dtype).convert<af::dtype>());
    POTHOS_TEST_EQUAL(dtype.size(), af::getSizeOf(afDType));

    auto dtypeFromAF = Pothos::Object(afDType).convert<Pothos::DType>();
    POTHOS_TEST_EQUAL(dtypeName, dtypeFromAF.name());

    testEnumValueConversion(dtypeName, afDType);
}

}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_backend_conversion)
{
    AFTests::testTypesCanConvert<std::string, af::Backend>();
    AFTests::testEnumValueConversion("CPU",    ::AF_BACKEND_CPU);
    AFTests::testEnumValueConversion("CUDA",   ::AF_BACKEND_CUDA);
    AFTests::testEnumValueConversion("OpenCL", ::AF_BACKEND_OPENCL);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_convmode_conversion)
{
    AFTests::testTypesCanConvert<std::string, af::convMode>();
    AFTests::testEnumValueConversion("Default", ::AF_CONV_DEFAULT);
    AFTests::testEnumValueConversion("Expand",  ::AF_CONV_EXPAND);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_convdomain_conversion)
{
    AFTests::testTypesCanConvert<std::string, af::convDomain>();
    AFTests::testEnumValueConversion("Auto",    ::AF_CONV_AUTO);
    AFTests::testEnumValueConversion("Spatial", ::AF_CONV_SPATIAL);
    AFTests::testEnumValueConversion("Freq",    ::AF_CONV_FREQ);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_randomenginetype_conversion)
{
    AFTests::testTypesCanConvert<std::string, af::randomEngineType>();
    AFTests::testEnumValueConversion("Philox",   ::AF_RANDOM_ENGINE_PHILOX);
    AFTests::testEnumValueConversion("Threefry", ::AF_RANDOM_ENGINE_THREEFRY);
    AFTests::testEnumValueConversion("Mersenne", ::AF_RANDOM_ENGINE_MERSENNE);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_topkfunction_conversion)
{
    AFTests::testTypesCanConvert<std::string, af::topkFunction>();
    AFTests::testEnumValueConversion("Min",     ::AF_TOPK_MIN);
    AFTests::testEnumValueConversion("Max",     ::AF_TOPK_MAX);
    AFTests::testEnumValueConversion("Default", ::AF_TOPK_DEFAULT);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_dtype_conversion)
{
    AFTests::testTypesCanConvert<Pothos::DType, af::dtype>();
    AFTests::testDTypeEnumUsage("int8",            ::b8);
    AFTests::testDTypeEnumUsage("int16",           ::s16);
    AFTests::testDTypeEnumUsage("int32",           ::s32);
    AFTests::testDTypeEnumUsage("int64",           ::s64);
    AFTests::testDTypeEnumUsage("uint8",           ::u8);
    AFTests::testDTypeEnumUsage("uint16",          ::u16);
    AFTests::testDTypeEnumUsage("uint32",          ::u32);
    AFTests::testDTypeEnumUsage("uint64",          ::u64);
    AFTests::testDTypeEnumUsage("float32",         ::f32);
    AFTests::testDTypeEnumUsage("float64",         ::f64);
    AFTests::testDTypeEnumUsage("complex_float32", ::c32);
    AFTests::testDTypeEnumUsage("complex_float64", ::c64);
}
