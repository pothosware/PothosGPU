// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <Poco/Format.h>

#include <iostream>
#include <vector>

struct TestBlockNames
{
    std::string pothosBlock;
    std::string pothosGPUBlock;
};

struct ValueCompareBlockParams
{
    Pothos::Proxy block;

    std::vector<std::string> sourceChans;
    std::vector<std::string> sinkChans;
};

struct ValueCompareParams
{
    ValueCompareBlockParams pothosBlockParams;
    ValueCompareBlockParams pothosGPUBlockParams;

    Pothos::DType sourceDType;
    Pothos::DType sinkDType;
};

static void compareIOBlockValues(const ValueCompareParams& params)
{
    std::cout << Poco::format(
                     " * Testing %s vs %s (%s -> %s)...",
                     params.pothosBlockParams.block.call<std::string>("getName"),
                     params.pothosGPUBlockParams.block.call<std::string>("getName"),
                     params.sourceDType.name(),
                     params.sinkDType.name()) << std::endl;

    POTHOS_TEST_EQUAL(
        params.pothosBlockParams.sourceChans.size(),
        params.pothosGPUBlockParams.sourceChans.size());
    POTHOS_TEST_EQUAL(
        params.pothosBlockParams.sinkChans.size(),
        params.pothosGPUBlockParams.sinkChans.size());

    const auto numSourceChans = params.pothosBlockParams.sourceChans.size();
    const auto numSinkChans = params.pothosBlockParams.sinkChans.size();

    std::vector<Pothos::Proxy> sources(numSourceChans);
    std::vector<Pothos::Proxy> pothosBlockSinks(numSinkChans);
    std::vector<Pothos::Proxy> pothosGPUBlockSinks(numSinkChans);

    for(size_t chan = 0; chan < numSourceChans; ++chan)
    {
        sources[chan] = Pothos::BlockRegistry::make(
                            "/blocks/feeder_source",
                            params.sourceDType);
        sources[chan].call(
            "feedBuffer",
            GPUTests::getTestInputs(params.sourceDType.name()));
    }
    for(size_t chan = 0; chan < numSinkChans; ++chan)
    {
        pothosBlockSinks[chan] = Pothos::BlockRegistry::make(
                                     "/blocks/collector_sink",
                                     params.sinkDType);
        pothosGPUBlockSinks[chan] = Pothos::BlockRegistry::make(
                                        "/blocks/collector_sink",
                                        params.sinkDType);
    }

    {
        Pothos::Topology topology;

        for(size_t chan = 0; chan < numSourceChans; ++chan)
        {
            topology.connect(
                sources[chan], 0,
                params.pothosBlockParams.block, params.pothosBlockParams.sourceChans[chan]);
            topology.connect(
                sources[chan], 0,
                params.pothosGPUBlockParams.block, params.pothosGPUBlockParams.sourceChans[chan]);
        }
        for(size_t chan = 0; chan < numSinkChans; ++chan)
        {
            topology.connect(
                params.pothosBlockParams.block, params.pothosBlockParams.sinkChans[chan],
                pothosBlockSinks[chan], 0);
            topology.connect(
                params.pothosGPUBlockParams.block, params.pothosGPUBlockParams.sinkChans[chan],
                pothosGPUBlockSinks[chan], 0);
        }

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    for(size_t chan = 0; chan < numSinkChans; ++chan)
    {
        std::cout << Poco::format(
                         "   * Testing output %s vs %s...",
                         params.pothosBlockParams.sinkChans[chan],
                         params.pothosGPUBlockParams.sinkChans[chan]) << std::endl;

        GPUTests::testBufferChunk(
            pothosBlockSinks[chan].call<Pothos::BufferChunk>("getBuffer"),
            pothosGPUBlockSinks[chan].call<Pothos::BufferChunk>("getBuffer"));
    }
}

POTHOS_TEST_BLOCK("/gpu/tests", compare_pothos_block_outputs)
{
    std::vector<TestBlockNames> oneChanFloatBlocks =
    {
        {"/blocks/floor", "/gpu/arith/floor"},
        {"/blocks/ceil", "/gpu/arith/ceil"},
        {"/blocks/trunc", "/gpu/arith/trunc"},
        {"/comms/gamma", "/gpu/arith/tgamma"},
        {"/comms/lngamma", "/gpu/arith/lgamma"},
    };

    for(const auto& block: oneChanFloatBlocks)
    {
        if(GPUTests::doesBlockExist(block.pothosBlock))
        {
            std::vector<Pothos::DType> dtypes = {"float32", "float64"};
            for(const auto& dtype: dtypes)
            {
                ValueCompareParams params =
                {
                    {
                        Pothos::BlockRegistry::make(block.pothosBlock, dtype),
                        {"0"},
                        {"0"}
                    },
                    {
                        Pothos::BlockRegistry::make(block.pothosGPUBlock, "Auto", dtype),
                        {"0"},
                        {"0"}
                    },

                    dtype,
                    dtype
                };
                compareIOBlockValues(params);
            }

        }
        else std::cout << "Could not find " << block.pothosBlock << ". Skipping." << std::endl;
    }
}
