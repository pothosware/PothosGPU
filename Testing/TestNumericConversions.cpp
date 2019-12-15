// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BlockExecutionTests.hpp" // TODO: move random input funcs to TestUtility
#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <iostream>
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

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_complex_conversion)
{
    testComplexConversion<std::complex<float>, af::cfloat>();
    testComplexConversion<std::complex<double>, af::cdouble>();
}

template <typename T>
static void testStdVectorToAfArrayConversion(af::dtype expectedAfDType)
{
    std::cout << "Testing " << Pothos::DType(typeid(T)).name() << "..." << std::endl;

    const std::vector<T> stdVector = getTestInputs<T>();

    const auto afArray = Pothos::Object(stdVector).convert<af::array>();
    POTHOS_TEST_EQUAL(1, afArray.numdims());
    POTHOS_TEST_TRUE(expectedAfDType == afArray.type());
    POTHOS_TEST_EQUAL(
        stdVector.size(),
        static_cast<size_t>(afArray.elements()));

    const auto stdVector2 = Pothos::Object(afArray).convert<std::vector<T>>();
    POTHOS_TEST_EQUALV(
        stdVector,
        stdVector2);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_std_vector_conversion)
{
    testStdVectorToAfArrayConversion<float>(::f32);
    testStdVectorToAfArrayConversion<double>(::f64);

    testStdVectorToAfArrayConversion<std::complex<float>>(::c32);
    testStdVectorToAfArrayConversion<std::complex<double>>(::c64);
}
