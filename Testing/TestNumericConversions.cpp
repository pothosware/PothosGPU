// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <iostream>
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

template <typename StdComplexType, typename AFComplexType>
static void testComplexConversion()
{
    testTypesCanConvert<StdComplexType, AFComplexType>();

    const StdComplexType stdComplex1{1.234, 5.678};
    const auto afComplex = Pothos::Object(stdComplex1).convert<AFComplexType>();

    testEqual(stdComplex1.real(), afComplex.real);
    testEqual(stdComplex1.imag(), afComplex.imag);

    const auto stdComplex2 = Pothos::Object(afComplex).convert<StdComplexType>();
    testEqual(stdComplex1, stdComplex2);
}

}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_complex_conversion)
{
    GPUTests::testComplexConversion<std::complex<float>, af::cfloat>();
    GPUTests::testComplexConversion<std::complex<double>, af::cdouble>();
}
