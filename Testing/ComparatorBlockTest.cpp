// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <arrayfire.h>

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
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

namespace PothosArrayFireTests
{

static constexpr const char* blockRegistryPath = "/arrayfire/array/comparator";

template <typename T, typename ComparatorFcn>
static void getTestValues(
    ComparatorFcn comparatorFcn,
    std::vector<T>* pinput0,
    std::vector<T>* pinput1,
    std::vector<std::int8_t>* pOutput)
{
    (*pinput0) = getTestInputs<T>(true /*shuffle*/);
    (*pinput1) = getTestInputs<T>(true /*shuffle*/);
    POTHOS_TEST_EQUAL(pinput0->size(), pinput1->size());

    for(size_t i = 0; i < pinput0->size(); ++i)
    {
        pOutput->emplace_back(comparatorFcn(pinput0->at(i), pinput1->at(i)) ? 1 : 0);
    }
    POTHOS_TEST_EQUAL(pOutput->size(), pinput0->size());
}

static void getTestValues(
    const std::string& type,
    const std::string& operation,
    Pothos::BufferChunk* pinput0,
    Pothos::BufferChunk* pinput1,
    Pothos::BufferChunk* pOutput)
{
    static const Pothos::DType Int8DType("int8");

    const Pothos::DType dtype(type);

#define GET_TEST_VALUES_FOR_OP(op, func) \
    if(operation == op) \
    { \
        getTestValues(func, &input0, &input1, &output); \
    } \

#define GET_TEST_VALUES(typeStr, cType) \
    if(type == typeStr) \
    { \
        std::vector<cType> input0, input1; \
        std::vector<std::int8_t> output; \
 \
        GET_TEST_VALUES_FOR_OP(">",  std::greater<cType>()); \
        GET_TEST_VALUES_FOR_OP(">=", std::greater_equal<cType>()); \
        GET_TEST_VALUES_FOR_OP("<",  std::less<cType>()); \
        GET_TEST_VALUES_FOR_OP("<=", std::less_equal<cType>()); \
        GET_TEST_VALUES_FOR_OP("==", std::equal_to<cType>()); \
        GET_TEST_VALUES_FOR_OP("!=", std::not_equal_to<cType>()); \
 \
        (*pinput0) = stdVectorToBufferChunk(dtype, input0); \
        (*pinput1) = stdVectorToBufferChunk(dtype, input1); \
        (*pOutput) = stdVectorToBufferChunk(Int8DType, output); \
        return; \
    }

    GET_TEST_VALUES("int16",   std::int16_t)
    GET_TEST_VALUES("int32",   std::int32_t)
    GET_TEST_VALUES("int64",   std::int64_t)
    GET_TEST_VALUES("uint8",   std::uint8_t)
    GET_TEST_VALUES("uint16",  std::uint16_t)
    GET_TEST_VALUES("uint32",  std::uint32_t)
    GET_TEST_VALUES("uint64",  std::uint64_t)
    GET_TEST_VALUES("float32", float)
    GET_TEST_VALUES("float64", double)
}

static void testComparatorBlockForTypeAndOperation(
    const std::string& type,
    const std::string& operation)
{
    std::cout << "Testing " << blockRegistryPath
              << " (type: " << type
              << ", operation: " << operation << ")" << std::endl;

    if(isDTypeComplexFloat(Pothos::DType(type)))
    {
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                blockRegistryPath,
                "Auto",
                type,
                operation),
            Pothos::ProxyExceptionMessage);
    }
    else
    {
        auto afComparator = Pothos::BlockRegistry::make(
                                blockRegistryPath,
                                "Auto",
                                type,
                                operation);
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
        getTestValues(
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

void testComparatorBlockForType(const std::string& type)
{
    static const std::vector<std::string> operations{"<","<=",">",">=","==","!="};
    for(const std::string& operation: operations)
    {
        testComparatorBlockForTypeAndOperation(type, operation);
    }
}

}
