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

    std::vector<Pothos::Proxy> subBlocks(numSinkChans);
    std::vector<Pothos::Proxy> meanBlocks(numSinkChans);
    std::vector<Pothos::Proxy> stdevBlocks(numSinkChans);
    std::vector<Pothos::Proxy> medianBlocks(numSinkChans);
    std::vector<Pothos::Proxy> medAbsDevBlocks(numSinkChans);

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
        subBlocks[chan] = Pothos::BlockRegistry::make(
                              "/comms/arithmetic",
                              params.sinkDType,
                              "SUB");
        meanBlocks[chan] = Pothos::BlockRegistry::make(
                               "/gpu/statistics/mean",
                               "Auto",
                               params.sinkDType);
        stdevBlocks[chan] = Pothos::BlockRegistry::make(
                                "/gpu/statistics/stdev",
                                "Auto",
                                params.sinkDType);
        medianBlocks[chan] = Pothos::BlockRegistry::make(
                                 "/gpu/statistics/median",
                                 "Auto",
                                 params.sinkDType);
        medAbsDevBlocks[chan] = Pothos::BlockRegistry::make(
                                    "/gpu/statistics/medabsdev",
                                    "Auto",
                                    params.sinkDType);
    }

    {
        Pothos::Topology topology;

        for(size_t chan = 0; chan < numSourceChans; ++chan)
        {
            topology.connect(
                sources[chan],
                0,
                params.pothosBlockParams.block,
                params.pothosBlockParams.sourceChans[chan]);
            topology.connect(
                sources[chan],
                0,
                params.pothosGPUBlockParams.block,
                params.pothosGPUBlockParams.sourceChans[chan]);
        }
        for(size_t chan = 0; chan < numSinkChans; ++chan)
        {
            topology.connect(
                params.pothosBlockParams.block,
                params.pothosBlockParams.sinkChans[chan],
                subBlocks[chan],
                0);
            topology.connect(
                params.pothosGPUBlockParams.block,
                params.pothosGPUBlockParams.sinkChans[chan],
                subBlocks[chan],
                1);

            topology.connect(
                subBlocks[chan], 0,
                meanBlocks[chan], 0);
            topology.connect(
                subBlocks[chan], 0,
                stdevBlocks[chan], 0);
            topology.connect(
                subBlocks[chan], 0,
                medianBlocks[chan], 0);
            topology.connect(
                subBlocks[chan], 0,
                medAbsDevBlocks[chan], 0);
        }

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    for(size_t chan = 0; chan < numSinkChans; ++chan)
    {
        const auto mean = meanBlocks[chan].call<double>("lastValue");
        const auto stdev = stdevBlocks[chan].call<double>("lastValue");
        const auto median = medianBlocks[chan].call<double>("lastValue");
        const auto medAbsDev = medAbsDevBlocks[chan].call<double>("lastValue");
        std::string outputString;

        if((0.0 != mean) || (0.0 != stdev))
        {
            outputString = Poco::format(
                               "   * %z mean difference:   %f +- %f",
                               chan,
                               mean,
                               stdev);
            std::cout << outputString << std::endl;
        }
        if((0.0 != median) || (0.0 != medAbsDev))
        {
            outputString = Poco::format(
                               "   * %z median difference: %f +- %f",
                               chan,
                               median,
                               medAbsDev);
            std::cout << outputString << std::endl;
        }
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
