// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <arrayfire.h>

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"
#include "Utility.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

static constexpr const char* arrayBlockRegistryPath = "/arrayfire/array/comparator";
static constexpr const char* scalarBlockRegistryPath = "/arrayfire/scalar/comparator";

template <typename T, typename ComparatorFcn>
static void getScalarTestValues(
    ComparatorFcn comparatorFcn,
    std::vector<T>* pInput,
    T* pScalar,
    std::vector<std::int8_t>* pOutput)
{
    (*pInput) = PothosArrayFireTests::getTestInputs<T>(true /*shuffle*/);
    (*pScalar) = PothosArrayFireTests::getSingleTestInput<T>();

    std::transform(
        pInput->begin(),
        pInput->end(),
        std::back_inserter(*pOutput),
        [&](T input)
        {
            return comparatorFcn(input, (*pScalar)) ? 1 : 0;
        });
    POTHOS_TEST_EQUAL(pOutput->size(), pInput->size());
}

template <typename T, typename ComparatorFcn>
static void getArrayTestValues(
    ComparatorFcn comparatorFcn,
    std::vector<T>* pInput0,
    std::vector<T>* pInput1,
    std::vector<std::int8_t>* pOutput)
{
    (*pInput0) = PothosArrayFireTests::getTestInputs<T>(true /*shuffle*/);
    (*pInput1) = PothosArrayFireTests::getTestInputs<T>(true /*shuffle*/);
    POTHOS_TEST_EQUAL(pInput0->size(), pInput1->size());

    for(size_t i = 0; i < pInput0->size(); ++i)
    {
        pOutput->emplace_back(comparatorFcn(pInput0->at(i), pInput1->at(i)) ? 1 : 0);
    }
    POTHOS_TEST_EQUAL(pOutput->size(), pInput0->size());
}

static void getScalarTestValues(
    const std::string& type,
    const std::string& operation,
    Pothos::BufferChunk* pInput,
    Pothos::Object* pScalar,
    Pothos::BufferChunk* pOutput)
{
    static const Pothos::DType Int8DType("int8");

    const Pothos::DType dtype(type);

#define GET_SCALAR_TEST_VALUES_FOR_OP(op, func) \
    if(operation == op) \
    { \
        getScalarTestValues(func, &input, &scalar, &output); \
    } \

#define GET_SCALAR_TEST_VALUES(typeStr, cType) \
    if(type == typeStr) \
    { \
        std::vector<cType> input; \
        cType scalar(0); \
        std::vector<std::int8_t> output; \
 \
        GET_SCALAR_TEST_VALUES_FOR_OP(">",  std::greater<cType>()); \
        GET_SCALAR_TEST_VALUES_FOR_OP(">=", std::greater_equal<cType>()); \
        GET_SCALAR_TEST_VALUES_FOR_OP("<",  std::less<cType>()); \
        GET_SCALAR_TEST_VALUES_FOR_OP("<=", std::less_equal<cType>()); \
        GET_SCALAR_TEST_VALUES_FOR_OP("==", std::equal_to<cType>()); \
        GET_SCALAR_TEST_VALUES_FOR_OP("!=", std::not_equal_to<cType>()); \
 \
        (*pInput) = PothosArrayFireTests::stdVectorToBufferChunk(input); \
        (*pScalar) = Pothos::Object(scalar); \
        (*pOutput) = PothosArrayFireTests::stdVectorToBufferChunk(output); \
        return; \
    }

    GET_SCALAR_TEST_VALUES("int16",   std::int16_t)
    GET_SCALAR_TEST_VALUES("int32",   std::int32_t)
    GET_SCALAR_TEST_VALUES("int64",   std::int64_t)
    GET_SCALAR_TEST_VALUES("uint8",   std::uint8_t)
    GET_SCALAR_TEST_VALUES("uint16",  std::uint16_t)
    GET_SCALAR_TEST_VALUES("uint32",  std::uint32_t)
    GET_SCALAR_TEST_VALUES("uint64",  std::uint64_t)
    GET_SCALAR_TEST_VALUES("float32", float)
    GET_SCALAR_TEST_VALUES("float64", double)
}

static void getArrayTestValues(
    const std::string& type,
    const std::string& operation,
    Pothos::BufferChunk* pInput0,
    Pothos::BufferChunk* pInput1,
    Pothos::BufferChunk* pOutput)
{
    static const Pothos::DType Int8DType("int8");

    const Pothos::DType dtype(type);

#define GET_ARRAY_TEST_VALUES_FOR_OP(op, func) \
    if(operation == op) \
    { \
        getArrayTestValues(func, &input0, &input1, &output); \
    } \

#define GET_ARRAY_TEST_VALUES(typeStr, cType) \
    if(type == typeStr) \
    { \
        std::vector<cType> input0, input1; \
        std::vector<std::int8_t> output; \
 \
        GET_ARRAY_TEST_VALUES_FOR_OP(">",  std::greater<cType>()); \
        GET_ARRAY_TEST_VALUES_FOR_OP(">=", std::greater_equal<cType>()); \
        GET_ARRAY_TEST_VALUES_FOR_OP("<",  std::less<cType>()); \
        GET_ARRAY_TEST_VALUES_FOR_OP("<=", std::less_equal<cType>()); \
        GET_ARRAY_TEST_VALUES_FOR_OP("==", std::equal_to<cType>()); \
        GET_ARRAY_TEST_VALUES_FOR_OP("!=", std::not_equal_to<cType>()); \
 \
        (*pInput0) = PothosArrayFireTests::stdVectorToBufferChunk(input0); \
        (*pInput1) = PothosArrayFireTests::stdVectorToBufferChunk(input1); \
        (*pOutput) = PothosArrayFireTests::stdVectorToBufferChunk(output); \
        return; \
    }

    GET_ARRAY_TEST_VALUES("int16",   std::int16_t)
    GET_ARRAY_TEST_VALUES("int32",   std::int32_t)
    GET_ARRAY_TEST_VALUES("int64",   std::int64_t)
    GET_ARRAY_TEST_VALUES("uint8",   std::uint8_t)
    GET_ARRAY_TEST_VALUES("uint16",  std::uint16_t)
    GET_ARRAY_TEST_VALUES("uint32",  std::uint32_t)
    GET_ARRAY_TEST_VALUES("uint64",  std::uint64_t)
    GET_ARRAY_TEST_VALUES("float32", float)
    GET_ARRAY_TEST_VALUES("float64", double)
}

static void testScalarComparatorBlockForTypeAndOperation(
    const std::string& type,
    const std::string& operation)
{
    std::cout << "Testing " << scalarBlockRegistryPath
              << " (type: " << type
              << ", operation: " << operation << ")" << std::endl;

    if(isDTypeComplexFloat(Pothos::DType(type)))
    {
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                scalarBlockRegistryPath,
                "Auto",
                operation,
                type,
                0),
            Pothos::ProxyExceptionMessage);
    }
    else
    {
        auto afComparator = Pothos::BlockRegistry::make(
                                scalarBlockRegistryPath,
                                "Auto",
                                operation,
                                type,
                                0);
        POTHOS_TEST_EQUAL(
            type,
            afComparator.call("input", 0).call("dtype").call<std::string>("name"));
        POTHOS_TEST_EQUAL(
            "int8",
            afComparator.call("output", 0).call("dtype").call<std::string>("name"));

        Pothos::BufferChunk input;
        Pothos::Object scalar;
        Pothos::BufferChunk output;
        getScalarTestValues(
            type,
            operation,
            &input,
            &scalar,
            &output);
        POTHOS_TEST_TRUE(input.elements() > 0);
        POTHOS_TEST_TRUE(scalar);
        POTHOS_TEST_EQUAL(input.elements(), output.elements());

        afComparator.call("setScalar", scalar);
        POTHOS_TEST_EQUAL(0, scalar.compareTo(afComparator.call("scalar")));

        auto feederSource = Pothos::BlockRegistry::make(
                                "/blocks/feeder_source",
                                type);
        auto collectorSink = Pothos::BlockRegistry::make(
                                 "/blocks/collector_sink",
                                 "int8");

        feederSource.call("feedBuffer", input);

        // Execute the topology.
        {
            Pothos::Topology topology;

            topology.connect(feederSource, 0, afComparator, 0);
            topology.connect(afComparator, 0, collectorSink, 0);

            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive(0.05));
        }

        PothosArrayFireTests::testBufferChunk(
            output,
            collectorSink.call<Pothos::BufferChunk>("getBuffer"));
    }
}

static void testArrayComparatorBlockForTypeAndOperation(
    const std::string& type,
    const std::string& operation)
{
    std::cout << "Testing " << arrayBlockRegistryPath
              << " (type: " << type
              << ", operation: " << operation << ")" << std::endl;

    if(isDTypeComplexFloat(Pothos::DType(type)))
    {
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                arrayBlockRegistryPath,
                "Auto",
                operation,
                type),
            Pothos::ProxyExceptionMessage);
    }
    else
    {
        auto afComparator = Pothos::BlockRegistry::make(
                                arrayBlockRegistryPath,
                                "Auto",
                                operation,
                                type);
        POTHOS_TEST_EQUAL(
            type,
            afComparator.call("input", 0).call("dtype").call<std::string>("name"));
        POTHOS_TEST_EQUAL(
            type,
            afComparator.call("input", 1).call("dtype").call<std::string>("name"));
        POTHOS_TEST_EQUAL(
            "int8",
            afComparator.call("output", 0).call("dtype").call<std::string>("name"));

        Pothos::BufferChunk input0;
        Pothos::BufferChunk input1;
        Pothos::BufferChunk output;
        getArrayTestValues(
            type,
            operation,
            &input0,
            &input1,
            &output);
        POTHOS_TEST_TRUE(input0.elements() > 0);
        POTHOS_TEST_EQUAL(input0.elements(), input1.elements());
        POTHOS_TEST_EQUAL(input0.elements(), output.elements());

        auto feederSource0 = Pothos::BlockRegistry::make(
                                 "/blocks/feeder_source",
                                 type);
        auto feederSource1 = Pothos::BlockRegistry::make(
                                 "/blocks/feeder_source",
                                 type);
        auto collectorSink = Pothos::BlockRegistry::make(
                                 "/blocks/collector_sink",
                                 "int8");

        feederSource0.call("feedBuffer", input0);
        feederSource1.call("feedBuffer", input1);

        // Execute the topology.
        {
            Pothos::Topology topology;

            topology.connect(feederSource0, 0, afComparator, 0);
            topology.connect(feederSource1, 0, afComparator, 1);
            topology.connect(afComparator, 0, collectorSink, 0);

            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive(0.05));
        }

        PothosArrayFireTests::testBufferChunk(
            output,
            collectorSink.call<Pothos::BufferChunk>("getBuffer"));
    }
}

static void testComparatorBlocksForType(const std::string& type)
{
    static const std::vector<std::string> operations{"<","<=",">",">=","==","!="};
    for(const std::string& operation: operations)
    {
        testScalarComparatorBlockForTypeAndOperation(type, operation);
        testArrayComparatorBlockForTypeAndOperation(type, operation);
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_comparators)
{
    PothosArrayFireTests::setupTestEnv();

    for(const auto& type: PothosArrayFireTests::getAllDTypeNames())
    {
        testComparatorBlocksForType(type);
    }
}
