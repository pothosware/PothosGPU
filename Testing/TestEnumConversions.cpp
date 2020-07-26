// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <string>
#include <typeinfo>

namespace GPUTests
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

POTHOS_TEST_BLOCK("/gpu/tests", test_af_backend_conversion)
{
    GPUTests::testTypesCanConvert<std::string, af::Backend>();
    GPUTests::testEnumValueConversion("CPU",    ::AF_BACKEND_CPU);
    GPUTests::testEnumValueConversion("CUDA",   ::AF_BACKEND_CUDA);
    GPUTests::testEnumValueConversion("OpenCL", ::AF_BACKEND_OPENCL);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_convmode_conversion)
{
    GPUTests::testTypesCanConvert<std::string, af::convMode>();
    GPUTests::testEnumValueConversion("Default", ::AF_CONV_DEFAULT);
    GPUTests::testEnumValueConversion("Expand",  ::AF_CONV_EXPAND);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_convdomain_conversion)
{
    GPUTests::testTypesCanConvert<std::string, af::convDomain>();
    GPUTests::testEnumValueConversion("Auto",    ::AF_CONV_AUTO);
    GPUTests::testEnumValueConversion("Spatial", ::AF_CONV_SPATIAL);
    GPUTests::testEnumValueConversion("Freq",    ::AF_CONV_FREQ);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_randomenginetype_conversion)
{
    GPUTests::testTypesCanConvert<std::string, af::randomEngineType>();
    GPUTests::testEnumValueConversion("Philox",   ::AF_RANDOM_ENGINE_PHILOX);
    GPUTests::testEnumValueConversion("Threefry", ::AF_RANDOM_ENGINE_THREEFRY);
    GPUTests::testEnumValueConversion("Mersenne", ::AF_RANDOM_ENGINE_MERSENNE);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_topkfunction_conversion)
{
    GPUTests::testTypesCanConvert<std::string, af::topkFunction>();
    GPUTests::testEnumValueConversion("Min",     ::AF_TOPK_MIN);
    GPUTests::testEnumValueConversion("Max",     ::AF_TOPK_MAX);
    GPUTests::testEnumValueConversion("Default", ::AF_TOPK_DEFAULT);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_dtype_conversion)
{
    GPUTests::testTypesCanConvert<Pothos::DType, af::dtype>();
    GPUTests::testDTypeEnumUsage("int8",            ::b8);
    GPUTests::testDTypeEnumUsage("int16",           ::s16);
    GPUTests::testDTypeEnumUsage("int32",           ::s32);
    GPUTests::testDTypeEnumUsage("int64",           ::s64);
    GPUTests::testDTypeEnumUsage("uint8",           ::u8);
    GPUTests::testDTypeEnumUsage("uint16",          ::u16);
    GPUTests::testDTypeEnumUsage("uint32",          ::u32);
    GPUTests::testDTypeEnumUsage("uint64",          ::u64);
    GPUTests::testDTypeEnumUsage("float32",         ::f32);
    GPUTests::testDTypeEnumUsage("float64",         ::f64);
    GPUTests::testDTypeEnumUsage("complex_float32", ::c32);
    GPUTests::testDTypeEnumUsage("complex_float64", ::c64);
}
