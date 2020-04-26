// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <arrayfire.h>

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"
#include "Utility.hpp"

#include <iostream>
#include <string>
#include <vector>

//
// Test implementations
//

static void testScalarOpBlock(
    const std::string& blockRegistryPath,
    const std::string& inputDType,
    const std::string& outputDType,
    const std::string& operation)
{
    if(inputDType == outputDType)
    {
        std::cout << "Testing " << operation << " (" << inputDType << ")" << std::endl;
    }
    else
    {
        std::cout << "Testing " << operation
                  << " (" << inputDType <<  " -> " << outputDType << ")" << std::endl;

    }

    const auto testInputs = AFTests::getTestInputs(inputDType);
    const auto scalar = AFTests::getSingleTestInput(inputDType);

    auto scalarOpBlock = Pothos::BlockRegistry::make(
                             blockRegistryPath,
                             "Auto",
                             operation,
                             inputDType,
                             scalar);

    auto feederSource = Pothos::BlockRegistry::make(
                            "/blocks/feeder_source",
                            inputDType);
    feederSource.call("feedBuffer", testInputs);

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             outputDType);


    {
        Pothos::Topology topology;

        topology.connect(
            feederSource, 0,
            scalarOpBlock, 0);
        topology.connect(
            scalarOpBlock, 0,
            collectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }
}

//
// Registered tests
//

POTHOS_TEST_BLOCK("/arrayfire/tests", test_scalar_arithmetic)
{
    const auto& allDTypeNames = AFTests::getAllDTypeNames();
    const std::vector<std::string> allOperations = {"Add", "Subtract", "Multiply", "Divide", "Modulus"};

    for(const auto& dtype: allDTypeNames)
    {
        for(const auto& operation: allOperations)
        {
            if(("Modulus" != operation) || !isDTypeComplexFloat(Pothos::DType(dtype)))
            {
                testScalarOpBlock(
                    "/arrayfire/scalar/arithmetic",
                    dtype,
                    dtype,
                    operation);
            }
        }
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_scalar_comparator)
{
    const std::vector<std::string> validDTypeNames =
    {
        "int16", "int32", "int64",
        "uint8", "uint16", "uint32", "uint64",
        "float32", "float64"
    };
    const std::vector<std::string> allOperations = {"<", "<=", ">", ">=", "==", "!="};

    for(const auto& dtype: validDTypeNames)
    {
        for(const auto& operation: allOperations)
        {
            testScalarOpBlock(
                "/arrayfire/scalar/comparator",
                dtype,
                "int8",
                operation);
        }
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_scalar_bitwise)
{
    const std::vector<std::string> validDTypeNames = {"int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64"};
    const std::vector<std::string> allOperations = {"And", "Or", "XOr", "Left Shift", "Right Shift"};

    for(const auto& dtype: validDTypeNames)
    {
        for(const auto& operation: allOperations)
        {
            testScalarOpBlock(
                "/arrayfire/scalar/bitwise",
                dtype,
                dtype,
                operation);
        }
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_scalar_logical)
{
    const std::vector<std::string> validDTypeNames = {"int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64"};
    const std::vector<std::string> allOperations = {"And", "Or"};

    for(const auto& dtype: validDTypeNames)
    {
        for(const auto& operation: allOperations)
        {
            testScalarOpBlock(
                "/arrayfire/scalar/logical",
                dtype,
                "int8",
                operation);
        }
    }
}
