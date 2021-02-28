// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>
#include <Pothos/Util/TypeInfo.hpp>

#include <arrayfire.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

template <typename EnumType>
static inline void testEnumToString(
    const std::string& stringVal,
    EnumType enumVal)
{
    POTHOS_TEST_EQUAL(stringVal, Pothos::Object(enumVal).toString());
}

template <typename AfArrayType>
static inline void testAfArrayToString(const AfArrayType& afArray)
{
    const auto afArrayStr = Pothos::Object(afArray).toString();

    const std::vector<std::string> expectedStrings =
    {
        Pothos::Util::typeInfoToString(typeid(AfArrayType)),
        Pothos::Object(af::getBackendId(afArray)).convert<std::string>()
    };

    // Instead of string matching, look for key parts.
    for(const auto& expectedStr: expectedStrings)
    {
        POTHOS_TEST_NOT_EQUAL(
            std::string::npos,
            afArrayStr.find(expectedStr));
    }
}

}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_backend_to_string)
{
    testEnumToString("CPU",    ::AF_BACKEND_CPU);
    testEnumToString("CUDA",   ::AF_BACKEND_CUDA);
    testEnumToString("OpenCL", ::AF_BACKEND_OPENCL);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_convmode_to_string)
{
    testEnumToString("Default", ::AF_CONV_DEFAULT);
    testEnumToString("Expand",  ::AF_CONV_EXPAND);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_convdomain_to_string)
{
    testEnumToString("Auto",    ::AF_CONV_AUTO);
    testEnumToString("Spatial", ::AF_CONV_SPATIAL);
    testEnumToString("Freq",    ::AF_CONV_FREQ);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_randomenginetype_to_string)
{
    testEnumToString("Philox",   ::AF_RANDOM_ENGINE_PHILOX);
    testEnumToString("Threefry", ::AF_RANDOM_ENGINE_THREEFRY);
    testEnumToString("Mersenne", ::AF_RANDOM_ENGINE_MERSENNE);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_topkfunction_to_string)
{
    testEnumToString("Min",     ::AF_TOPK_MIN);
    testEnumToString("Max",     ::AF_TOPK_MAX);
    testEnumToString("Default", ::AF_TOPK_DEFAULT);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_dtype_to_string)
{
    testEnumToString("int8",            ::b8);
    testEnumToString("int16",           ::s16);
    testEnumToString("int32",           ::s32);
    testEnumToString("int64",           ::s64);
    testEnumToString("uint8",           ::u8);
    testEnumToString("uint16",          ::u16);
    testEnumToString("uint32",          ::u32);
    testEnumToString("uint64",          ::u64);
    testEnumToString("float32",         ::f32);
    testEnumToString("float64",         ::f64);
    testEnumToString("complex_float32", ::c32);
    testEnumToString("complex_float64", ::c64);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_array_to_string)
{
    af::array arr(3, 6, 7, ::f32);
    testAfArrayToString(arr);
    testAfArrayToString(arr(0,0));
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_array_compare)
{
    auto afArrayObj0 = Pothos::Object(af::randu(3, 6, 7, ::s32));
    auto afArrayObj1 = Pothos::Object(afArrayObj0.extract<af::array>().copy());
    POTHOS_TEST_EQUAL(0, afArrayObj0.compareTo(afArrayObj1));

    afArrayObj1.ref<af::array>()(1,2,3) += 5;
    POTHOS_TEST_NOT_EQUAL(0, afArrayObj0.compareTo(afArrayObj1));
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_array_serialization)
{
    Pothos::Object inputObj(af::randu(1, 2, 3, 4, ::c64));

    std::stringstream stream;
    inputObj.serialize(stream);

    Pothos::Object outputObj;
    outputObj.deserialize(stream);
    POTHOS_TEST_EQUAL(0, inputObj.compareTo(outputObj));
}
