// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>
#include <Pothos/Proxy.hpp>

#include <algorithm>
#include <iostream>
#include <set>
#include <vector>

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
static void testSetUnion()
{
    const auto dtype = Pothos::DType(typeid(T));

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    constexpr size_t numChannels = 3;
    std::vector<Pothos::BufferChunk> inputs;
    std::vector<Pothos::Proxy> sources;
    for(size_t i = 0; i < numChannels; ++i)
    {
        inputs.emplace_back(GPUTests::getTestInputs(dtype.name()));

        sources.emplace_back(Pothos::BlockRegistry::make("/blocks/feeder_source", dtype));
        sources.back().call("feedBuffer", inputs.back());
    }
    auto expectedOutput = getBufferChunkSetUnion<T>(inputs);

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

POTHOS_TEST_BLOCK("/gpu/tests", test_set_union)
{
    // TODO: enable chars in type-support
    //testSetUnion<char>();
    testSetUnion<short>();
    //testSetUnion<int>();
    //testSetUnion<long long>();
    //testSetUnion<unsigned char>();
    testSetUnion<unsigned short>();
    //testSetUnion<unsigned int>();
    //testSetUnion<unsigned long long>();
    testSetUnion<float>();
    testSetUnion<double>();
}
