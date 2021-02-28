// Copyright (c) 2020-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>
#include <Pothos/Proxy.hpp>

#include <Poco/Random.h>
#include <Poco/Timestamp.h>

#include <algorithm>
#include <iostream>
#include <set>
#include <vector>

// To avoid collisions
namespace
{

constexpr size_t numChannels = 3;

template <typename T>
static Pothos::BufferChunk getBufferChunkSetUnion(const std::vector<Pothos::BufferChunk>& bufferChunks)
{
    POTHOS_TEST_FALSE(bufferChunks.empty());

    std::set<T> setUnion;
    for(const auto& bufferChunk: bufferChunks)
    {
        std::copy(
            bufferChunk.as<T*>(),
            bufferChunk.as<T*>()+bufferChunk.elements(),
            std::inserter(setUnion, setUnion.end()));
    }

    std::vector<T> setUnionVec;
    std::copy(
        setUnion.begin(),
        setUnion.end(),
        std::back_inserter(setUnionVec));

    return GPUTests::stdVectorToBufferChunk(setUnionVec);
}

template <typename T>
static void getSetUnionTestValues(
    std::vector<Pothos::BufferChunk>* pInputs,
    Pothos::BufferChunk* pOutput)
{
    const auto dtype = Pothos::DType(typeid(T));

    for(size_t chan = 0; chan < numChannels; ++chan)
    {
        pInputs->emplace_back(GPUTests::getTestInputs(dtype.name()));
    }

    // Duplicate some values from each buffer in all the others.
    constexpr size_t maxNumRepeats = 10;
    for(size_t srcChan = 0; srcChan < numChannels; ++srcChan)
    {
        const T* srcBuffer = pInputs->at(srcChan);

        for(size_t dstChan = 0; dstChan < numChannels; ++dstChan)
        {
            if(srcChan == dstChan) continue;

            const auto srcIndex = Poco::Random().next(GPUTests::TestInputLength);
            const auto repeatCount = Poco::Random().next(maxNumRepeats)+1;
            T* dstBuffer = pInputs->at(dstChan);

            for(size_t rep = 0; rep < repeatCount; ++rep)
            {
                const auto dstIndex = Poco::Random().next(GPUTests::TestInputLength);
                dstBuffer[dstIndex] = srcBuffer[srcIndex];
            }
        }
    }

    auto setUnion = getBufferChunkSetUnion<T>(*pInputs);
    POTHOS_TEST_LT(setUnion.elements(), (pInputs->size() * pInputs->at(0).elements()));

    (*pOutput) = std::move(setUnion);
}

template <typename T>
static void testSetUnion()
{
    const auto dtype = Pothos::DType(typeid(T));

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    std::vector<Pothos::BufferChunk> inputs;
    Pothos::BufferChunk expectedOutput;
    getSetUnionTestValues<T>(&inputs, &expectedOutput);

    std::vector<Pothos::Proxy> sources(numChannels);
    for(size_t i = 0; i < numChannels; ++i)
    {
        sources[i] = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
        sources[i].call("feedBuffer", inputs[i]);
    }

    auto setUnion = Pothos::BlockRegistry::make("/gpu/algorithm/set_union", "Auto", dtype, numChannels);
    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        for(size_t chan = 0; chan < numChannels; ++chan)
        {
            topology.connect(sources[chan], 0, setUnion, chan);
        }
        topology.connect(setUnion, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    GPUTests::testBufferChunk(
        expectedOutput,
        sink.call<Pothos::BufferChunk>("getBuffer"));
}

// Note: not testing floats due to differences in precision
// between different ArrayFire backends. Assumption is that
// integral types are enough to test.
POTHOS_TEST_BLOCK("/gpu/tests", test_set_union)
{
    af::setSeed(Poco::Timestamp().utcTime());

    testSetUnion<char>();
    testSetUnion<signed char>();
    testSetUnion<short>();
    testSetUnion<int>();
    testSetUnion<long long>();
    testSetUnion<unsigned char>();
    testSetUnion<unsigned short>();
    testSetUnion<unsigned>();
    testSetUnion<unsigned long long>();
}

}
