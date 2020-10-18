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

template <typename T>
static bool isClose(T t0, T t1)
{
    return (std::abs(t0-t1) < 1e-6);
}

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

        if(!isClose(mean, 0.0) || !isClose(stdev, 0.0) || !isClose(median, 0.0) || !isClose(medAbsDev, 0.0))
        {
            outputString = Poco::format(
                               "   * %z mean difference:   %f +- %f",
                               chan,
                               mean,
                               stdev);
            std::cout << outputString << std::endl;

            outputString = Poco::format(
                               "   * %z median difference: %f +- %f",
                               chan,
                               median,
                               medAbsDev);
            std::cout << outputString << std::endl;
        }
    }
}

static void testTrigBlock(
    const std::string& pothosGPUBlockPath,
    const std::string& pothosCommsOperation)
{
    std::vector<Pothos::DType> dtypes = {"float32", "float64"};
    for(const auto& dtype: dtypes)
    {
        ValueCompareParams params =
        {
            {
                Pothos::BlockRegistry::make("/comms/trigonometric", dtype, pothosCommsOperation),
                {"0"},
                {"0"}
            },
            {
                Pothos::BlockRegistry::make(pothosGPUBlockPath, "Auto", dtype),
                {"0"},
                {"0"}
            },

            dtype,
            dtype
        };
        compareIOBlockValues(params);
    }
}

static void testConverterBlock(const Pothos::DType& inputDType)
{
    std::vector<Pothos::DType> outputDTypes = {"int32", "int64", "uint32", "uint64", "float32", "float64"};
    for(const auto& outputDType: outputDTypes)
    {
        ValueCompareParams params =
        {
            {
                Pothos::BlockRegistry::make("/blocks/converter", outputDType),
                {"0"},
                {"0"}
            },
            {
                Pothos::BlockRegistry::make("/gpu/array/cast", "Auto", inputDType, outputDType),
                {"0"},
                {"0"}
            },

            inputDType,
            outputDType
        };
        compareIOBlockValues(params);
    }
}

POTHOS_TEST_BLOCK("/gpu/tests", compare_pothos_block_outputs)
{
    std::vector<TestBlockNames> oneChanFloatBlocks =
    {
        {"/comms/abs", "/gpu/arith/abs"},
        {"/blocks/floor", "/gpu/arith/floor"},
        {"/blocks/ceil", "/gpu/arith/ceil"},
        {"/blocks/trunc", "/gpu/arith/trunc"},
        {"/comms/gamma", "/gpu/arith/tgamma"},
        {"/comms/lngamma", "/gpu/arith/lgamma"},
        {"/comms/sinc", "/gpu/signal/sinc"},
        {"/comms/log1p", "/gpu/arith/log1p"},
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
        else std::cout << " * Could not find " << block.pothosBlock << ". Skipping." << std::endl;
    }

    if(GPUTests::doesBlockExist("/comms/trigonometric"))
    {
        testTrigBlock("/gpu/arith/cos", "COS");
        testTrigBlock("/gpu/arith/sin", "SIN");
        testTrigBlock("/gpu/arith/tan", "TAN");
        testTrigBlock("/gpu/arith/sec", "SEC");
        testTrigBlock("/gpu/arith/csc", "CSC");
        testTrigBlock("/gpu/arith/cot", "COT");
        testTrigBlock("/gpu/arith/acos", "ACOS");
        testTrigBlock("/gpu/arith/asin", "ASIN");
        testTrigBlock("/gpu/arith/atan", "ATAN");
        testTrigBlock("/gpu/arith/asec", "ASEC");
        testTrigBlock("/gpu/arith/acsc", "ACSC");
        testTrigBlock("/gpu/arith/acot", "ACOT");
        testTrigBlock("/gpu/arith/cosh", "COSH");
        testTrigBlock("/gpu/arith/sinh", "SINH");
        testTrigBlock("/gpu/arith/tanh", "TANH");
        testTrigBlock("/gpu/arith/sech", "SECH");
        testTrigBlock("/gpu/arith/csch", "CSCH");
        testTrigBlock("/gpu/arith/coth", "COTH");
        testTrigBlock("/gpu/arith/acosh", "ACOSH");
        testTrigBlock("/gpu/arith/asinh", "ASINH");
        testTrigBlock("/gpu/arith/atanh", "ATANH");
        testTrigBlock("/gpu/arith/asech", "ASECH");
        testTrigBlock("/gpu/arith/acsch", "ACSCH");
        testTrigBlock("/gpu/arith/acoth", "ACOTH");
    }
    else std::cout << " * Could not find /comms/trigonometric. Skipping." << std::endl;

    if(GPUTests::doesBlockExist("/blocks/converter"))
    {
        testConverterBlock("int16");
        testConverterBlock("int32");
        testConverterBlock("int64");
        testConverterBlock("uint8");
        testConverterBlock("uint16");
        testConverterBlock("uint32");
        testConverterBlock("uint64");
        testConverterBlock("float32");
        testConverterBlock("float64");
    }
    else std::cout << " * Could not find /blocks/converter. Skipping." << std::endl;
}
