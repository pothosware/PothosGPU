// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace PothosArrayFireTests
{

template <typename T>
static Pothos::BufferChunk getIsInfNaNTestInputs()
{
    const std::vector<T> testInputs =
    {
        0.0,
        std::numeric_limits<T>::infinity(),
        -std::numeric_limits<T>::infinity(),
        5.0,
        std::numeric_limits<T>::quiet_NaN(),
    };
    return stdVectorToBufferChunk(
               Pothos::DType(typeid(T)),
               testInputs);
}

void testIsInfNaNBlockForType(const std::string& type)
{
    static constexpr const char* isInfBlockRegistryPath = "/arrayfire/arith/isinf";
    static constexpr const char* isNaNBlockRegistryPath = "/arrayfire/arith/isnan";

    const Pothos::DType dtype(type);
    if(isDTypeFloat(dtype))
    {
        std::cout << "Testing " << isInfBlockRegistryPath
                  << " and " << isNaNBlockRegistryPath
                  << " (type: " << type << ")" << std::endl;

        static const Pothos::DType Int8DType("int8");

        auto testInputs = ("float32" == type) ? getIsInfNaNTestInputs<float>()
                                              : getIsInfNaNTestInputs<double>();

        static const auto expectedIsInfOutput = std::vector<std::int8_t>{0,1,1,0,0};
        static const auto expectedIsNaNOutput = std::vector<std::int8_t>{0,0,0,0,1};

        auto feederSource = Pothos::BlockRegistry::make("/blocks/feeder_source", type);
        feederSource.call("feedBuffer", testInputs);

        auto isInf = Pothos::BlockRegistry::make(
                         isInfBlockRegistryPath,
                         "Auto",
                         type);
        auto isNaN = Pothos::BlockRegistry::make(
                         isNaNBlockRegistryPath,
                         "Auto",
                         type);
        
        auto isInfCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", Int8DType);
        auto isNaNCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", Int8DType);

        {
            Pothos::Topology topology;

            topology.connect(feederSource, 0, isInf, 0);
            topology.connect(isInf, 0, isInfCollectorSink, 0);

            topology.connect(feederSource, 0, isNaN, 0);
            topology.connect(isNaN, 0, isNaNCollectorSink, 0);

            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive(0.05));
        }

        std::cout << " * Testing isinf..." << std::endl;
        testBufferChunk<std::int8_t>(
            isInfCollectorSink.call("getBuffer"),
            expectedIsInfOutput);
        std::cout << " * Testing isnan..." << std::endl;
        testBufferChunk<std::int8_t>(
            isNaNCollectorSink.call("getBuffer"),
            expectedIsNaNOutput);
    }
    else
    {
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                isInfBlockRegistryPath,
                "Auto",
                type),
        Pothos::ProxyExceptionMessage);
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                isNaNBlockRegistryPath,
                "Auto",
                type),
        Pothos::ProxyExceptionMessage);
    }
}

}
