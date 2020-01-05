// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Util/TypeInfo.hpp>

#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include <arrayfire.h>

#include <memory>

template <typename AfArrayType>
static Pothos::BufferChunk afArrayTypeToBufferChunk(const AfArrayType& afArray)
{
    auto afArraySPtr = std::make_shared<af::array>(afArray);
    const size_t address = reinterpret_cast<size_t>(afArraySPtr->template device<std::uint8_t>());
    const size_t bytes = afArraySPtr->bytes();

    Pothos::SharedBuffer sharedBuff(address, bytes, afArraySPtr);
    Pothos::BufferChunk bufferChunk(sharedBuff);
    bufferChunk.dtype = Pothos::Object(afArray.type()).convert<Pothos::DType>();
    if(bufferChunk.elements() != static_cast<size_t>(afArraySPtr->elements()))
    {
        throw Pothos::AssertionViolationException(
                  Poco::format(
                      "Element count doesn't match in %s conversion to Pothos::BufferChunk",
                      Pothos::Util::typeInfoToString(typeid(AfArrayType))),
                  Poco::format(
                      "%s -> %s",
                      Poco::NumberFormatter::format(afArraySPtr->elements()),
                      bufferChunk.elements()));
    }

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
