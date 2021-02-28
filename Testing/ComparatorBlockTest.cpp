// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

// To avoid collisions
namespace
{

static constexpr const char* arrayBlockRegistryPath = "/gpu/array/comparator";
static constexpr const char* scalarBlockRegistryPath = "/gpu/scalar/comparator";

template <typename T, typename ComparatorFcn>
static void getScalarTestValues(
    ComparatorFcn comparatorFcn,
    Pothos::BufferChunk* pInput,
    Pothos::Object* pScalar,
    Pothos::BufferChunk* pOutput)
{
    const auto dtype = Pothos::DType(typeid(T));

    (*pInput) = GPUTests::getTestInputs(dtype.name());
    (*pScalar) = GPUTests::getRandomValue(*pInput);
    (*pOutput) = Pothos::BufferChunk("int8", pInput->elements());

    const T* inputBuffer = pInput->as<const T*>();
    const T& scalar = pScalar->extract<T>();
    char* outputBuffer = pOutput->as<char*>();

    for(size_t i = 0; i < pInput->elements(); ++i)
    {
        outputBuffer[i] = comparatorFcn(inputBuffer[i], scalar) ? 1 : 0;
    }
}

template <typename T, typename ComparatorFcn>
static void getArrayTestValues(
    ComparatorFcn comparatorFcn,
    Pothos::BufferChunk* pInput0,
    Pothos::BufferChunk* pInput1,
    Pothos::BufferChunk* pOutput)
{
    const auto dtype = Pothos::DType(typeid(T));

    (*pInput0) = GPUTests::getTestInputs(dtype.name());
    (*pInput1) = GPUTests::getTestInputs(dtype.name());
    POTHOS_TEST_EQUAL(pInput0->elements(), pInput1->elements());
    (*pOutput) = Pothos::BufferChunk("int8", pInput0->elements());

    const T* inputBuffer0 = pInput0->as<const T*>();
    const T* inputBuffer1 = pInput1->as<const T*>();
    char* outputBuffer = pOutput->as<char*>();

    for(size_t i = 0; i < pInput0->elements(); ++i)
    {
        outputBuffer[i] = comparatorFcn(inputBuffer0[i], inputBuffer1[i]) ? 1 : 0;
    }
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

#define GET_SCALAR_TEST_VALUES_FOR_OP(op, cType, func) \
    if(operation == op) \
    { \
        getScalarTestValues<cType>(func, pInput, pScalar, pOutput); \
    } \

#define GET_SCALAR_TEST_VALUES(typeStr, cType) \
    if(type == typeStr) \
    { \
 \
        GET_SCALAR_TEST_VALUES_FOR_OP(">",  cType, std::greater<cType>()); \
        GET_SCALAR_TEST_VALUES_FOR_OP(">=", cType, std::greater_equal<cType>()); \
        GET_SCALAR_TEST_VALUES_FOR_OP("<",  cType, std::less<cType>()); \
        GET_SCALAR_TEST_VALUES_FOR_OP("<=", cType, std::less_equal<cType>()); \
        GET_SCALAR_TEST_VALUES_FOR_OP("==", cType, std::equal_to<cType>()); \
        GET_SCALAR_TEST_VALUES_FOR_OP("!=", cType, std::not_equal_to<cType>()); \
 \
        return; \
    }

    GET_SCALAR_TEST_VALUES("int8",    char)
    GET_SCALAR_TEST_VALUES("int16",   short)
    GET_SCALAR_TEST_VALUES("int32",   int)
    GET_SCALAR_TEST_VALUES("int64",   long long)
    GET_SCALAR_TEST_VALUES("uint8",   unsigned char)
    GET_SCALAR_TEST_VALUES("uint16",  unsigned short)
    GET_SCALAR_TEST_VALUES("uint32",  unsigned)
    GET_SCALAR_TEST_VALUES("uint64",  unsigned long long)
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

#define GET_ARRAY_TEST_VALUES_FOR_OP(op, cType, func) \
    if(operation == op) \
    { \
        getArrayTestValues<cType>(func, pInput0, pInput1, pOutput); \
    } \

#define GET_ARRAY_TEST_VALUES(typeStr, cType) \
    if(type == typeStr) \
    { \
 \
        GET_ARRAY_TEST_VALUES_FOR_OP(">",  cType, std::greater<cType>()); \
        GET_ARRAY_TEST_VALUES_FOR_OP(">=", cType, std::greater_equal<cType>()); \
        GET_ARRAY_TEST_VALUES_FOR_OP("<",  cType, std::less<cType>()); \
        GET_ARRAY_TEST_VALUES_FOR_OP("<=", cType, std::less_equal<cType>()); \
        GET_ARRAY_TEST_VALUES_FOR_OP("==", cType, std::equal_to<cType>()); \
        GET_ARRAY_TEST_VALUES_FOR_OP("!=", cType, std::not_equal_to<cType>()); \
 \
        return; \
    }

    GET_ARRAY_TEST_VALUES("int8",    char)
    GET_ARRAY_TEST_VALUES("int16",   short)
    GET_ARRAY_TEST_VALUES("int32",   int)
    GET_ARRAY_TEST_VALUES("int64",   long long)
    GET_ARRAY_TEST_VALUES("uint8",   unsigned char)
    GET_ARRAY_TEST_VALUES("uint16",  unsigned short)
    GET_ARRAY_TEST_VALUES("uint32",  unsigned)
    GET_ARRAY_TEST_VALUES("uint64",  unsigned long long)
    GET_ARRAY_TEST_VALUES("float32", float)
    GET_ARRAY_TEST_VALUES("float64", double)
}

static void testScalarComparatorBlockForTypeAndOperation(
    const Pothos::DType& type,
    const std::string& operation)
{
    std::cout << "Testing " << scalarBlockRegistryPath
              << " (type: " << type.name()
              << ", operation: " << operation << ")" << std::endl;

    if(PothosGPU::isDTypeComplexFloat(Pothos::DType(type)))
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
            type.name(),
            operation,
            &input,
            &scalar,
            &output);
        POTHOS_TEST_GT(input.elements(), 0);
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

        GPUTests::testBufferChunk(
            output,
            collectorSink.call<Pothos::BufferChunk>("getBuffer"));
    }
}

static void testArrayComparatorBlockForTypeAndOperation(
    const Pothos::DType& type,
    const std::string& operation)
{
    std::cout << "Testing " << arrayBlockRegistryPath
              << " (type: " << type.name()
              << ", operation: " << operation << ")" << std::endl;

    if(PothosGPU::isDTypeComplexFloat(Pothos::DType(type)))
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
            type.name(),
            operation,
            &input0,
            &input1,
            &output);
        POTHOS_TEST_GT(input0.elements(), 0);
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

        GPUTests::testBufferChunk(
            output,
            collectorSink.call<Pothos::BufferChunk>("getBuffer"));
    }
}

static void testComparatorBlocksForType(const Pothos::DType& type)
{
    static const std::vector<std::string> operations{"<","<=",">",">=","==","!="};
    for(const std::string& operation: operations)
    {
        testScalarComparatorBlockForTypeAndOperation(type, operation);
        testArrayComparatorBlockForTypeAndOperation(type, operation);
    }
}

POTHOS_TEST_BLOCK("/gpu/tests", test_comparators)
{
    GPUTests::setupTestEnv();

    for(const auto& type: GPUTests::getAllDTypes())
    {
        testComparatorBlocksForType(type);
    }
}

}
