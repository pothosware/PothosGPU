// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <complex>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{
    constexpr size_t numBins = 2 << 16;
    std::random_device rd;
    std::mt19937 g(rd());

    template <typename T>
    EnableIfFloat<T, std::vector<T>> getFFTInputs()
    {
        auto inputs = GPUTests::linspace<T>(-30.f, 20.f, numBins);
        std::shuffle(inputs.begin(), inputs.end(), g);

        return inputs;
    }

    template <typename T>
    GPUTests::EnableIfComplex<T, std::vector<T>> getFFTInputs()
    {
        using Scalar = typename T::value_type;

        auto inputs = GPUTests::toComplexVector(GPUTests::linspace<Scalar>(-30.f, 20.f, numBins*2));
        std::shuffle(inputs.begin(), inputs.end(), g);

        return inputs;
    }

    Pothos::BufferChunk getFFTInputs(const std::string& type)
    {
        #define IfTypeGetFFTInputs(typeStr, cType) \
            if(typeStr == type) return GPUTests::stdVectorToBufferChunk(getFFTInputs<cType>());

        IfTypeGetFFTInputs("float32", float)
        else IfTypeGetFFTInputs("float64", double)
        else IfTypeGetFFTInputs("complex_float32", std::complex<float>)
        else IfTypeGetFFTInputs("complex_float64", std::complex<double>)

        // Should never get here
        return Pothos::BufferChunk();
    }

    struct TestParams
    {
        std::string fwdInputType;
        std::string fwdOutputType;
        bool inverse;
    };

    static void testFFT(const TestParams& testParams)
    {
        std::cout << " * Testing " << testParams.fwdInputType
                  << " -> " << testParams.fwdOutputType
                  << " (inverse: " << std::boolalpha << testParams.inverse << ")" << std::endl;

        static constexpr double norm = 10.0;

        const auto& feederSourceType = testParams.inverse ? testParams.fwdOutputType
                                                          : testParams.fwdInputType;
        const auto& collectorSinkType = testParams.inverse ? testParams.fwdInputType
                                                           : testParams.fwdOutputType;

        auto feederSource = Pothos::BlockRegistry::make(
                                "/blocks/feeder_source",
                                feederSourceType);
        feederSource.call("feedBuffer", getFFTInputs(feederSourceType));

        auto collectorSink = Pothos::BlockRegistry::make(
                                 "/blocks/collector_sink",
                                 collectorSinkType);

        auto fft = Pothos::BlockRegistry::make(
                       "/gpu/signal/fft",
                       "Auto",
                       testParams.fwdInputType,
                       testParams.fwdOutputType,
                       numBins,
                       norm,
                       testParams.inverse);
        POTHOS_TEST_EQUAL(norm, fft.call("normalizationFactor"));

        {
            Pothos::Topology topology;
            topology.connect(
                feederSource, 0,
                fft, 0);
            topology.connect(
                fft, 0,
                collectorSink, 0);

            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive());
        }
    }
}

POTHOS_TEST_BLOCK("/gpu/tests", test_fft)
{
    const std::vector<TestParams> allTestParams =
    {
        {"float32", "complex_float32", false},
        {"complex_float32", "float32", false},
        {"complex_float32", "float32", true},
        {"complex_float32", "complex_float32", false},
        {"complex_float32", "complex_float32", true},
        {"float64", "complex_float64", false},
        {"complex_float64", "float64", false},
        {"complex_float64", "float64", true},
        {"complex_float64", "complex_float64", false},
        {"complex_float64", "complex_float64", true},
    };
    for(const auto& testParams: allTestParams)
    {
        testFFT(testParams);
    }
}
