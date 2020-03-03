// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <string>
#include <typeinfo>

namespace PothosArrayFireTests
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

    auto dtypeFromAF = Pothos::Object(afDType).convert<Pothos::DType>();
    POTHOS_TEST_EQUAL(dtypeName, dtypeFromAF.name());

    testEnumValueConversion(dtypeName, afDType);
}

}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_backend_conversion)
{
    PothosArrayFireTests::testTypesCanConvert<std::string, af::Backend>();
    PothosArrayFireTests::testEnumValueConversion("CPU",    ::AF_BACKEND_CPU);
    PothosArrayFireTests::testEnumValueConversion("CUDA",   ::AF_BACKEND_CUDA);
    PothosArrayFireTests::testEnumValueConversion("OpenCL", ::AF_BACKEND_OPENCL);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_convmode_conversion)
{
    PothosArrayFireTests::testTypesCanConvert<std::string, af::convMode>();
    PothosArrayFireTests::testEnumValueConversion("Default", ::AF_CONV_DEFAULT);
    PothosArrayFireTests::testEnumValueConversion("Expand",  ::AF_CONV_EXPAND);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_convdomain_conversion)
{
    PothosArrayFireTests::testTypesCanConvert<std::string, af::convDomain>();
    PothosArrayFireTests::testEnumValueConversion("Auto",    ::AF_CONV_AUTO);
    PothosArrayFireTests::testEnumValueConversion("Spatial", ::AF_CONV_SPATIAL);
    PothosArrayFireTests::testEnumValueConversion("Freq",    ::AF_CONV_FREQ);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_randomenginetype_conversion)
{
    PothosArrayFireTests::testTypesCanConvert<std::string, af::randomEngineType>();
    PothosArrayFireTests::testEnumValueConversion("Philox",   ::AF_RANDOM_ENGINE_PHILOX);
    PothosArrayFireTests::testEnumValueConversion("Threefry", ::AF_RANDOM_ENGINE_THREEFRY);
    PothosArrayFireTests::testEnumValueConversion("Mersenne", ::AF_RANDOM_ENGINE_MERSENNE);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_dtype_conversion)
{
    PothosArrayFireTests::testTypesCanConvert<Pothos::DType, af::dtype>();
    PothosArrayFireTests::testDTypeEnumUsage("int8",            ::b8);
    PothosArrayFireTests::testDTypeEnumUsage("int16",           ::s16);
    PothosArrayFireTests::testDTypeEnumUsage("int32",           ::s32);
    PothosArrayFireTests::testDTypeEnumUsage("int64",           ::s64);
    PothosArrayFireTests::testDTypeEnumUsage("uint8",           ::u8);
    PothosArrayFireTests::testDTypeEnumUsage("uint16",          ::u16);
    PothosArrayFireTests::testDTypeEnumUsage("uint32",          ::u32);
    PothosArrayFireTests::testDTypeEnumUsage("uint64",          ::u64);
    PothosArrayFireTests::testDTypeEnumUsage("float32",         ::f32);
    PothosArrayFireTests::testDTypeEnumUsage("float64",         ::f64);
    PothosArrayFireTests::testDTypeEnumUsage("complex_float32", ::c32);
    PothosArrayFireTests::testDTypeEnumUsage("complex_float64", ::c64);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_interptype_conversion)
{
    PothosArrayFireTests::testTypesCanConvert<std::string, af::interpType>();
    PothosArrayFireTests::testEnumValueConversion("Nearest",         ::AF_INTERP_NEAREST);
    PothosArrayFireTests::testEnumValueConversion("Linear",          ::AF_INTERP_LINEAR);
    PothosArrayFireTests::testEnumValueConversion("Bilinear",        ::AF_INTERP_BILINEAR);
    PothosArrayFireTests::testEnumValueConversion("Cubic",           ::AF_INTERP_CUBIC);
    PothosArrayFireTests::testEnumValueConversion("Lower",           ::AF_INTERP_LOWER);
    PothosArrayFireTests::testEnumValueConversion("Linear Cosine",   ::AF_INTERP_LINEAR_COSINE);
    PothosArrayFireTests::testEnumValueConversion("Bilinear Cosine", ::AF_INTERP_BILINEAR_COSINE);
    PothosArrayFireTests::testEnumValueConversion("Bicubic",         ::AF_INTERP_BICUBIC);
    PothosArrayFireTests::testEnumValueConversion("Cubic Spline",    ::AF_INTERP_CUBIC_SPLINE);
    PothosArrayFireTests::testEnumValueConversion("Bicubic Spline",  ::AF_INTERP_BICUBIC_SPLINE);
}
