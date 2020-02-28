// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"
#include "Utility.hpp"

namespace PothosArrayFireTests
{

#define RETURN_BUFFERCHUNK(typeStr, cType) \
    if(type == typeStr) \
        return stdVectorToBufferChunk(getTestInputs<cType>());

Pothos::BufferChunk getTestInputs(const std::string& type)
{
    // ArrayFire doesn't support int8
    RETURN_BUFFERCHUNK("int16", std::int16_t)
    RETURN_BUFFERCHUNK("int32", std::int32_t)
    RETURN_BUFFERCHUNK("int64", std::int64_t)
    RETURN_BUFFERCHUNK("uint8", std::uint8_t)
    RETURN_BUFFERCHUNK("uint16", std::uint16_t)
    RETURN_BUFFERCHUNK("uint32", std::uint32_t)
    RETURN_BUFFERCHUNK("uint64", std::uint64_t)
    RETURN_BUFFERCHUNK("float32", float)
    RETURN_BUFFERCHUNK("float64", double)
    // ArrayFire doesn't support any integral complex type
    RETURN_BUFFERCHUNK("complex_float32", std::complex<float>)
    RETURN_BUFFERCHUNK("complex_float64", std::complex<double>)

    // Should never happen
    return Pothos::BufferChunk();
}

#define RETURN_OBJECT(typeStr, cType) \
    if(type == typeStr) \
        return Pothos::Object(getSingleTestInput<cType>());

Pothos::Object getSingleTestInput(const std::string& type)
{
    // ArrayFire doesn't support int8
    RETURN_OBJECT("int16", std::int16_t)
    RETURN_OBJECT("int32", std::int32_t)
    RETURN_OBJECT("int64", std::int64_t)
    RETURN_OBJECT("uint8", std::uint8_t)
    RETURN_OBJECT("uint16", std::uint16_t)
    RETURN_OBJECT("uint32", std::uint32_t)
    RETURN_OBJECT("uint64", std::uint64_t)
    RETURN_OBJECT("float32", float)
    RETURN_OBJECT("float64", double)
    // ArrayFire doesn't support any integral complex type
    RETURN_OBJECT("complex_float32", std::complex<float>)
    RETURN_OBJECT("complex_float64", std::complex<double>)

    // Should never happen
    return Pothos::Object();
}

const std::vector<std::string>& getAllDTypeNames()
{
    static const std::vector<std::string> AllTypes =
    {
        // ArrayFire doesn't support int8
        "int16",
        "int32",
        "int64",
        "uint8",
        "uint16",
        "uint32",
        "uint64",
        "float32",
        "float64",
        // ArrayFire doesn't support complex integral types
        "complex_float32",
        "complex_float64"
    };
    return AllTypes;
}

}
