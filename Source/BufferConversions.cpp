// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BufferConversions.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Util/TypeInfo.hpp>

#include <Poco/Format.h>
#include <Poco/Logger.h>
#include <Poco/NumberFormatter.h>

#include <arrayfire.h>

#include <memory>
#include <vector>

//
// Minimal wrapper class to ensure allocation and deallocation are done
// with the same backend.
//

AfPinnedMemRAII::AfPinnedMemRAII(af::Backend backend, size_t allocSize):
    _backend(backend),
    _pinnedMem(nullptr)
{
    af::setBackend(_backend);
    _pinnedMem = af::pinned(allocSize, ::u8);
}

AfPinnedMemRAII::~AfPinnedMemRAII()
{
    try
    {
        af::setBackend(_backend);
        af::freePinned(_pinnedMem);
    }
    catch(...){}
}

//
// Pothos::BufferChunk <-> af::array
//

template <typename AfArrayType>
Pothos::BufferChunk afArrayTypeToBufferChunk(const AfArrayType& afArray)
{
    af::array outputAfArray(afArray);
    return moveAfArrayToBufferChunk(std::move(outputAfArray));
}

Pothos::BufferChunk moveAfArrayToBufferChunk(af::array&& rAfArray)
{
    #ifndef NDEBUG
    static auto& logger = Poco::Logger::get(__FUNCTION__);
    logger.setLevel("debug");

    poco_debug_f3(
        logger,
        "Moving %s af::array of size %s and type %s",
        Pothos::Object(af::getBackendId(rAfArray)).convert<std::string>(),
        Poco::NumberFormatter::format(rAfArray.bytes()),
        Pothos::Object(rAfArray.type()).convert<std::string>());
    #endif

    return Pothos::BufferChunk();
}

static af::array bufferChunkToAfArray(const Pothos::BufferChunk& bufferChunk)
{
    return af::array();
}

//
// std::vector <-> af::array
//

template <typename T>
static af::array convertStdVectorToAfArray(const std::vector<T>& vec)
{
    // If this is ever used for types where type and PothosToAF::type are
    // different, we need two versions.
    static_assert(sizeof(T) == sizeof(typename PothosToAF<T>::type));

    return af::array(
               static_cast<dim_t>(vec.size()),
               reinterpret_cast<const typename PothosToAF<T>::type*>(vec.data()),
               ::afHost);
}

template <typename Num, typename Arr>
static std::vector<Num> convertAfArrayToStdVector(const Arr& arr)
{
    assert(Pothos::DType(typeid(Num)) == Pothos::Object(arr.type()).convert<Pothos::DType>());

    std::vector<Num> ret(arr.elements());
    arr.host(ret.data());
    return ret;
}

template <typename T>
static void registerStdVectorConversion(const std::string& typeName)
{
    static const std::string convertPluginSubpath("/object/convert/arrayfire");

    const std::string stdVectorToAFArrayPluginPath =
        Poco::format(
            "%s/vec%s_to_af_array",
            convertPluginSubpath,
            typeName);
    const std::string afArrayToStdVectorPluginPath =
        Poco::format(
            "%s/af_array_to_vec%s",
            convertPluginSubpath,
            typeName);
    const std::string afArrayProxyToStdVectorPluginPath =
        Poco::format(
            "%s/af_arrayproxy_to_vec%s",
            convertPluginSubpath,
            typeName);

    Pothos::PluginRegistry::add(
        stdVectorToAFArrayPluginPath,
        Pothos::Callable(convertStdVectorToAfArray<T>));
    Pothos::PluginRegistry::add(
        afArrayToStdVectorPluginPath,
        Pothos::Callable(convertAfArrayToStdVector<T,af::array>));
    Pothos::PluginRegistry::add(
        afArrayProxyToStdVectorPluginPath,
        Pothos::Callable(convertAfArrayToStdVector<T,af::array::array_proxy>));
}

pothos_static_block(registerArrayFireBufferConversions)
{
    Pothos::PluginRegistry::add(
        "/object/convert/arrayfire/afarray_to_bufferchunk",
        Pothos::Callable(&afArrayTypeToBufferChunk<af::array>));
    Pothos::PluginRegistry::add(
        "/object/convert/arrayfire/afarrayproxy_to_bufferchunk",
        Pothos::Callable(&afArrayTypeToBufferChunk<af::array::array_proxy>));
    Pothos::PluginRegistry::add(
        "/object/convert/arrayfire/bufferchunk_to_afarray",
        Pothos::Callable(&bufferChunkToAfArray));

    registerStdVectorConversion<float>("float");
    registerStdVectorConversion<double>("double");

    registerStdVectorConversion<std::complex<float>>("cfloat");
    registerStdVectorConversion<std::complex<double>>("cdouble");
}
