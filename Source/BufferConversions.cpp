// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <Pothos/Framework.hpp>
#include <Pothos/Plugin.hpp>

#include <arrayfire.h>

#include <memory>

static Pothos::BufferChunk afArrayToBufferChunk(const af::array& afArray)
{
    // The type is arbitrary, but there is no void* implementation, so
    // attempting to use it results in a linker error.
    size_t address = reinterpret_cast<size_t>(afArray.device<std::uint8_t>());
    size_t numBytes = afArray.bytes();
    auto sharedAfArray = std::shared_ptr<af::array>(new af::array(afArray));

    Pothos::BufferChunk bufferChunk(Pothos::SharedBuffer(address, numBytes, sharedAfArray));
    bufferChunk.dtype = Pothos::DType(Pothos::Object(afArray.type()).convert<std::string>());

    return bufferChunk;
}

static Pothos::BufferChunk afArrayProxyToBufferChunk(const af::array::array_proxy& afArrayProxy)
{
    return afArrayToBufferChunk(afArrayProxy);
}

static af::array bufferChunkToAfArray(const Pothos::BufferChunk& bufferChunk)
{
    dim_t dim0 = static_cast<dim_t>(bufferChunk.elements());
    auto afDType = Pothos::Object(bufferChunk.dtype.name()).convert<::af_dtype>();

    // TODO: is there a way to detect if the incoming BufferChunk references
    // device memory allocated by ArrayFire? If so, we can just return an
    // af::array referencing the same array.

    // The type is arbitrary, but there is no void* implementation, so
    // attempting to use it results in a linker error.
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
        Pothos::Callable(&afArrayToBufferChunk));
    Pothos::PluginRegistry::add(
        "/object/convert/arrayfire/afarrayproxy_to_bufferchunk",
        Pothos::Callable(&afArrayProxyToBufferChunk));
    Pothos::PluginRegistry::add(
        "/object/convert/arrayfire/bufferchunk_to_afarray",
        Pothos::Callable(&bufferChunkToAfArray));
}
