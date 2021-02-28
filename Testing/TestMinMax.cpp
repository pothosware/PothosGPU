// Copyright (c) 2020-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

static constexpr size_t numInputs = 3;

template <typename T>
static void getTestParams(
    std::vector<Pothos::BufferChunk>* pTestInputsOut,
    Pothos::BufferChunk* pExpectedMinOutputsOut,
    Pothos::BufferChunk* pExpectedMaxOutputsOut)
{
    std::vector<std::vector<T>> inputVecs =
    {
        std::vector<T>{std::numeric_limits<T>::min(), 0,10,20,30,40,50},
        std::vector<T>{std::numeric_limits<T>::max(), 55,45,35,25,15,5},
        std::vector<T>{2,45,35,25,27,30,45},
    };

    std::vector<T> minOutputVec(inputVecs[0].size());
    std::vector<T> maxOutputVec(inputVecs[0].size());
    for(size_t elem = 0; elem < minOutputVec.size(); ++elem)
    {
        std::vector<T> elems{inputVecs[0][elem], inputVecs[1][elem], inputVecs[2][elem]};
        auto minmaxElems = std::minmax_element(elems.begin(), elems.end());

        minOutputVec[elem] = *minmaxElems.first;
        maxOutputVec[elem] = *minmaxElems.second;
    }

    std::transform(
        inputVecs.begin(),
        inputVecs.end(),
        std::back_inserter(*pTestInputsOut),
        [](const std::vector<T>& inputVec)
        {
            return GPUTests::stdVectorToBufferChunk(inputVec);
        });

    *pExpectedMinOutputsOut = GPUTests::stdVectorToBufferChunk(minOutputVec);
    *pExpectedMaxOutputsOut = GPUTests::stdVectorToBufferChunk(maxOutputVec);
}

template <typename T>
static void testMinMax()
{
    const Pothos::DType dtype(typeid(T));
    
    std::cout << "Testing " << dtype.name() << std::endl;

    auto min = Pothos::BlockRegistry::make(
                   "/gpu/arith/min",
                   "Auto",
                   dtype,
                   numInputs);
    auto max = Pothos::BlockRegistry::make(
                   "/gpu/arith/max",
                   "Auto",
                   dtype,
                   numInputs);

    std::vector<Pothos::Proxy> feederSources;
    for(size_t i = 0; i < numInputs; ++i)
    {
        feederSources.emplace_back(Pothos::BlockRegistry::make("/blocks/feeder_source", dtype));
    }

    auto minCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto maxCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    std::vector<Pothos::BufferChunk> inputs;
    Pothos::BufferChunk expectedMinOutputs;
    Pothos::BufferChunk expectedMaxOutputs;
    getTestParams<T>(
        &inputs,
        &expectedMinOutputs,
        &expectedMaxOutputs);
    POTHOS_TEST_EQUAL(numInputs, inputs.size());

    {
        Pothos::Topology topology;
        for(size_t chanIn = 0; chanIn < numInputs; ++chanIn)
        {
            feederSources[chanIn].call("feedBuffer", inputs[chanIn]);
            topology.connect(feederSources[chanIn], 0, min, chanIn);
            topology.connect(feederSources[chanIn], 0, max, chanIn);
        }

        topology.connect(min, 0, minCollectorSink, 0);
        topology.connect(max, 0, maxCollectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    std::cout << " * Checking min..." << std::endl;
    GPUTests::testBufferChunk(
        expectedMinOutputs,
        minCollectorSink.call("getBuffer"));
    std::cout << " * Checking max..." << std::endl;
    GPUTests::testBufferChunk(
        expectedMaxOutputs,
        maxCollectorSink.call("getBuffer"));
}

POTHOS_TEST_BLOCK("/gpu/tests", test_min_max)
{
    testMinMax<char>();
    testMinMax<short>();
    testMinMax<int>();
    testMinMax<long long>();
    testMinMax<unsigned char>();
    testMinMax<unsigned short>();
    testMinMax<unsigned>();
    testMinMax<unsigned long long>();
    testMinMax<float>();
    testMinMax<double>();
}
