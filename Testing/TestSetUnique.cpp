// Copyright (c) 2020 Nicholas Corgan
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

template <typename T>
static void getSetUniqueTestValues(
    Pothos::BufferChunk* pInput,
    Pothos::BufferChunk* pOutput)
{
    const auto dtype = Pothos::DType(typeid(T));

    auto inputVector = GPUTests::bufferChunkToStdVector<T>(GPUTests::getTestInputs(dtype.name()));
    const auto originalSize = inputVector.size();

    // Duplicate some values.
    const auto numDuplicates = inputVector.size() / 10;
    constexpr size_t maxNumRepeats = 10;
    for(size_t dup = 0; dup < numDuplicates; ++dup)
    {
        const auto index = Poco::Random().next(Poco::UInt32(originalSize));
        const auto repeatCount = Poco::Random().next(maxNumRepeats)+1;

        for(size_t rep = 0; rep < repeatCount; ++rep)
        {
            inputVector.emplace_back(inputVector[index]);
        }
    }

    std::set<T> intermediate;
    std::copy(
        inputVector.begin(),
        inputVector.end(),
        std::inserter(intermediate, intermediate.end()));

    std::vector<T> outputVector;
    std::copy(
        intermediate.begin(),
        intermediate.end(),
        std::back_inserter(outputVector));

    (*pInput) = GPUTests::stdVectorToBufferChunk(inputVector);
    (*pOutput) = GPUTests::stdVectorToBufferChunk(outputVector);
}

template <typename T>
static void testSetUnique()
{
    const auto dtype = Pothos::DType(typeid(T));

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    Pothos::BufferChunk input;
    Pothos::BufferChunk expectedOutput;
    getSetUniqueTestValues<T>(&input, &expectedOutput);

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    source.call("feedBuffer", input);

    auto setUnique = Pothos::BlockRegistry::make(
                         "/gpu/algorithm/set_unique",
                         "Auto",
                         dtype);

    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(source, 0, setUnique, 0);
        topology.connect(setUnique, 0, sink, 0);

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
//
// TODO: figure out 64-bit integral issues
POTHOS_TEST_BLOCK("/gpu/tests", test_set_unique)
{
    af::setSeed(Poco::Timestamp().utcTime());

    // TODO: reenable char
    //testSetUnique<char>();
    testSetUnique<short>();
    testSetUnique<int>();
    //testSetUnique<long long>();
    testSetUnique<unsigned char>();
    testSetUnique<unsigned short>();
    testSetUnique<unsigned int>();
    //testSetUnique<unsigned long long>();
}
