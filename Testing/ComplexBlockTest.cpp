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

#include <Poco/Thread.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <typeinfo>
#include <vector>

namespace
{

constexpr const char* combineRegistryPath        = "/arrayfire/arith/combine_complex";
constexpr const char* splitRegistryPath          = "/arrayfire/arith/split_complex";
constexpr const char* polarToComplexRegistryPath = "/arrayfire/arith/polar_to_complex";
constexpr const char* complexToPolarRegistryPath = "/arrayfire/arith/complex_to_polar";

// These calls involve multiple kernels, so give them some initial compile time.
constexpr int sleepTimeMs = 500;

template <typename T>
Pothos::BufferChunk getPhaseInputs()
{
    static std::random_device rd;
    static std::mt19937 g(rd());
    
    auto phases = PothosArrayFireTests::linspace<T>(-M_PI/2, M_PI/2, 123);
    std::shuffle(phases.begin(), phases.end(), g);

    return PothosArrayFireTests::stdVectorToBufferChunk(phases);
}

inline Pothos::BufferChunk getPhaseInputs(const std::string& type)
{
    return ("float32" == type) ? getPhaseInputs<float>() : getPhaseInputs<double>();
}

// Since multiple kernels are involved, we'll be more forgiving for floating-point errors.
void testBufferChunksEqual(
    const Pothos::BufferChunk& expectedBufferChunk,
    const Pothos::BufferChunk& actualBufferChunk)
{
    POTHOS_TEST_EQUAL(
        expectedBufferChunk.dtype.name(),
        actualBufferChunk.dtype.name());
    POTHOS_TEST_EQUAL(
        expectedBufferChunk.elements(),
        actualBufferChunk.elements());

    constexpr double epsilon = 1e-3;

    if("float32" == expectedBufferChunk.dtype.name())
    {
        for(size_t elem = 0; elem < expectedBufferChunk.elements(); ++elem)
        {
            POTHOS_TEST_CLOSE(
                expectedBufferChunk.as<const float*>()[elem],
                actualBufferChunk.as<const float*>()[elem],
                epsilon);
        }
    }
    else if("float64" == expectedBufferChunk.dtype.name())
    {
        for(size_t elem = 0; elem < expectedBufferChunk.elements(); ++elem)
        {
            POTHOS_TEST_CLOSE(
                expectedBufferChunk.as<const double*>()[elem],
                actualBufferChunk.as<const double*>()[elem],
                epsilon);
        }
    }
    else if("complex_float32" == expectedBufferChunk.dtype.name())
    {
        for(size_t elem = 0; elem < expectedBufferChunk.elements(); ++elem)
        {
            POTHOS_TEST_CLOSE(
                expectedBufferChunk.as<const std::complex<float>*>()[elem].real(),
                actualBufferChunk.as<const std::complex<float>*>()[elem].real(),
                epsilon);
            POTHOS_TEST_CLOSE(
                expectedBufferChunk.as<const std::complex<float>*>()[elem].imag(),
                actualBufferChunk.as<const std::complex<float>*>()[elem].imag(),
                epsilon);
        }
    }
    else if("complex_float64" == expectedBufferChunk.dtype.name())
    {
        for(size_t elem = 0; elem < expectedBufferChunk.elements(); ++elem)
        {
            POTHOS_TEST_CLOSE(
                expectedBufferChunk.as<const std::complex<double>*>()[elem].real(),
                actualBufferChunk.as<const std::complex<double>*>()[elem].real(),
                epsilon);
            POTHOS_TEST_CLOSE(
                expectedBufferChunk.as<const std::complex<double>*>()[elem].imag(),
                actualBufferChunk.as<const std::complex<double>*>()[elem].imag(),
                epsilon);
        }
    }
    else
    {
        PothosArrayFireTests::testBufferChunk(
            expectedBufferChunk,
            actualBufferChunk);
    }
}

void testScalarToComplexToScalar(
    const std::string& scalarToComplexRegistryPath,
    const std::string& complexToScalarRegistryPath,
    const std::string& port0Name,
    const std::string& port1Name,
    const std::string& type)
{
    std::cout << "Testing " << scalarToComplexRegistryPath << " -> " << complexToScalarRegistryPath
              << " (type: " << type << ")" << std::endl;

    auto port0TestInputs = PothosArrayFireTests::getTestInputs(type);
    auto port1TestInputs = getPhaseInputs(type); // Inputs don't matter for other function

    auto port0FeederSource = Pothos::BlockRegistry::make(
                                 "/blocks/feeder_source",
                                 type);
    port0FeederSource.call("feedBuffer", port0TestInputs);

    auto port1FeederSource = Pothos::BlockRegistry::make(
                                 "/blocks/feeder_source",
                                 type);
    port1FeederSource.call("feedBuffer", port1TestInputs);

    auto scalarToComplex = Pothos::BlockRegistry::make(
                               scalarToComplexRegistryPath,
                               "Auto",
                               type);
    auto complexToScalar = Pothos::BlockRegistry::make(
                               complexToScalarRegistryPath,
                               "Auto",
                               type);

    auto port0CollectorSink = Pothos::BlockRegistry::make(
                                  "/blocks/collector_sink",
                                  type);
    auto port1CollectorSink = Pothos::BlockRegistry::make(
                                  "/blocks/collector_sink",
                                  type);

    {
        Pothos::Topology topology;

        topology.connect(
            port0FeederSource, 0,
            scalarToComplex, port0Name);
        topology.connect(
            port1FeederSource, 0,
            scalarToComplex, port1Name);

        topology.connect(
            scalarToComplex, 0,
            complexToScalar, 0);

        topology.connect(
            complexToScalar, port0Name,
            port0CollectorSink, 0);
        topology.connect(
            complexToScalar, port1Name,
            port1CollectorSink, 0);

        topology.commit();
        Poco::Thread::sleep(sleepTimeMs);
        POTHOS_TEST_TRUE(topology.waitInactive());
    }
    POTHOS_TEST_TRUE(port0CollectorSink.call("getBuffer").call<size_t>("elements") > 0);
    POTHOS_TEST_TRUE(port1CollectorSink.call("getBuffer").call<size_t>("elements") > 0);

    testBufferChunksEqual(
        port0TestInputs,
        port0CollectorSink.call("getBuffer"));
    testBufferChunksEqual(
        port1TestInputs,
        port1CollectorSink.call("getBuffer"));
}

void testComplexToScalarToComplex(
    const std::string& scalarToComplexRegistryPath,
    const std::string& complexToScalarRegistryPath,
    const std::string& port0Name,
    const std::string& port1Name,
    const std::string& type)
{
    std::cout << "Testing " << complexToScalarRegistryPath << " -> " << scalarToComplexRegistryPath
              << " (type: " << type << ")" << std::endl;

    const auto complexType = "complex_"+type;

    auto testInputs = PothosArrayFireTests::getTestInputs(complexType);

    auto feederSource = Pothos::BlockRegistry::make(
                            "/blocks/feeder_source",
                            complexType);
    feederSource.call("feedBuffer", testInputs);

    auto complexToScalar = Pothos::BlockRegistry::make(
                               complexToScalarRegistryPath,
                               "Auto",
                               type);
    auto scalarToComplex = Pothos::BlockRegistry::make(
                               scalarToComplexRegistryPath,
                               "Auto",
                               type);

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             complexType);

    {
        Pothos::Topology topology;

        topology.connect(
            feederSource, 0,
            complexToScalar, 0);

        topology.connect(
            complexToScalar, port0Name,
            scalarToComplex, port0Name);
        topology.connect(
            complexToScalar, port1Name,
            scalarToComplex, port1Name);

        topology.connect(
            scalarToComplex, 0,
            collectorSink, 0);

        topology.commit();
        Poco::Thread::sleep(sleepTimeMs);
        POTHOS_TEST_TRUE(topology.waitInactive());
    }
    POTHOS_TEST_TRUE(collectorSink.call("getBuffer").call<size_t>("elements") > 0);

    testBufferChunksEqual(
        testInputs,
        collectorSink.call("getBuffer"));
}

void testCombineToSplit(const std::string& type)
{
    testScalarToComplexToScalar(
        combineRegistryPath,
        splitRegistryPath,
        "re",
        "im",
        type);
}

void testSplitToCombine(const std::string& type)
{
    testComplexToScalarToComplex(
        combineRegistryPath,
        splitRegistryPath,
        "re",
        "im",
        type);
}

void testPolarToComplexToPolar(const std::string& type)
{
    testScalarToComplexToScalar(
        polarToComplexRegistryPath,
        complexToPolarRegistryPath,
        "mag",
        "phase",
        type);
}

void testComplexToPolarToComplex(const std::string& type)
{
    testComplexToScalarToComplex(
        polarToComplexRegistryPath,
        complexToPolarRegistryPath,
        "mag",
        "phase",
        type);
}

}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_complex_blocks)
{
    PothosArrayFireTests::setupTestEnv();

    const std::vector<std::string> dtypes = {"float32","float64"};
    
    for(const auto& type: dtypes)
    {
        testCombineToSplit(type);
        testSplitToCombine(type);
        testPolarToComplexToPolar(type);
        testComplexToPolarToComplex(type);
    }
}
