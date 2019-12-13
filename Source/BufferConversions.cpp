// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <Pothos/Framework.hpp>
#include <Pothos/Plugin.hpp>

#include <arrayfire.h>

#include <memory>

template <typename AfArrayType>
static Pothos::BufferChunk afArrayTypeToBufferChunk(const AfArrayType& afArray)
{
    const auto dtype = Pothos::Object(afArray.type()).convert<Pothos::DType>();
    const auto elements = afArray.elements();

    Pothos::BufferChunk bufferChunk(dtype, elements);
    afArray.host(reinterpret_cast<void*>(bufferChunk.address));
    return bufferChunk;
}

static af::array bufferChunkToAfArray(const Pothos::BufferChunk& bufferChunk)
{
    const auto dim0 = static_cast<dim_t>(bufferChunk.elements());
    const auto afDType = Pothos::Object(bufferChunk.dtype).convert<af::dtype>();

    af::array afArray(dim0, afDType);
    afArray.write<std::uint8_t>(
        bufferChunk.as<const std::uint8_t*>(),
        bufferChunk.length);

    return afArray;
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
}
