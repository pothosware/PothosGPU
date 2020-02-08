// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "BufferConversions.hpp"
#include "DeviceCache.hpp"
#include "Utility.hpp"

#include <nlohmann/json.hpp>

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>
#include <Poco/Logger.h>
#include <Poco/NumberFormatter.h>

#include <arrayfire.h>

#include <algorithm>
#include <string>

ArrayFireBlock::ArrayFireBlock(const std::string& device):
    Pothos::Block(),
    _afDeviceName(device)
{
    const auto& deviceCache = getDeviceCache();
    if(device == "Auto")
    {
        _afBackend = deviceCache[0].afBackendEnum;
        _afDevice = deviceCache[0].afDeviceIndex;
        _afDeviceName = deviceCache[0].name;

#if IS_AF_CONFIG_PER_THREAD
        af::setBackend(_afBackend);
        af::setDevice(_afDevice);
#endif
    }
    else
    {
        auto deviceCacheIter = std::find_if(
                                   deviceCache.begin(),
                                   deviceCache.end(),
                                   [&device](const DeviceCacheEntry& entry)
                                   {
                                       return (entry.name == device);
                                   });
        if(deviceCache.end() != deviceCacheIter)
        {
            _afBackend = deviceCacheIter->afBackendEnum;
            _afDevice = deviceCacheIter->afDeviceIndex;

#if IS_AF_CONFIG_PER_THREAD
            af::setBackend(_afBackend);
            af::setDevice(_afDevice);
#endif
        }
        else
        {
            throw Pothos::InvalidArgumentException(
                      Poco::format(
                          "Could not find ArrayFire device %s.",
                          device));
        }
    }

    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, getArrayFireBackend));
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, getArrayFireDevice));
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, overlay));
}

ArrayFireBlock::~ArrayFireBlock()
{
}

Pothos::BufferManager::Sptr ArrayFireBlock::getInputBufferManager(
    const std::string& /*name*/,
    const std::string& domain)
{
    if((domain == this->getPortDomain()) || domain.empty())
    {
        // We always want to operate on pinned memory, as GPUs can access this via DMA.
        Pothos::BufferManagerArgs args;
        args.numBuffers = 16;
        args.bufferSize = (2 << 20);
        
        return Pothos::BufferManager::make("pinned", args);
    }

    throw Pothos::PortDomainError(domain);
}

Pothos::BufferManager::Sptr ArrayFireBlock::getOutputBufferManager(
    const std::string& /*name*/,
    const std::string& domain)
{
    if((domain == this->getPortDomain()) || domain.empty())
    {
        // We always want to operate on pinned memory, as GPUs can access this via DMA.
        Pothos::BufferManagerArgs args;
        args.numBuffers = 16;
        args.bufferSize = (2 << 20);
        
        return Pothos::BufferManager::make("pinned", args);
    }

    throw Pothos::PortDomainError(domain);
}

std::string ArrayFireBlock::getArrayFireBackend() const
{
    return Pothos::Object(_afBackend).convert<std::string>();
}

std::string ArrayFireBlock::getArrayFireDevice() const
{
    return _afDeviceName;
}

std::string ArrayFireBlock::getPortDomain() const
{
    return Poco::format(
               "ArrayFire_%s",
               Pothos::Object(_afBackend).convert<std::string>());
}

std::string ArrayFireBlock::overlay() const
{
    nlohmann::json topObj;
    auto& params = topObj["params"];

    nlohmann::json deviceParam;
    deviceParam["key"] = "device";
    deviceParam["widgetType"] = "ComboBox";
    deviceParam["widgetKwargs"]["editable"] = false;

    auto& deviceParamOpts = deviceParam["options"];

    // The default option defaults to what the library guesses is the most
    // optimal device.
    nlohmann::json defaultOption;
    defaultOption["name"] = "Auto";
    defaultOption["value"] = "\"Auto\"";
    deviceParamOpts.push_back(defaultOption);

    for(const auto& entry: getDeviceCache())
    {
        nlohmann::json option;
        option["name"] = Poco::format(
                             "%s (%s)",
                             entry.name,
                             Pothos::Object(entry.afBackendEnum).convert<std::string>());
        option["value"] = Poco::format("\"%s\"", entry.name);
        deviceParamOpts.push_back(option);
    }

    params.push_back(deviceParam);

    return topObj.dump();
}

//
// Input port API
//

af::array ArrayFireBlock::getInputPortAsAfArray(
    size_t portNum,
    bool truncateToMinLength)
{
    return _getInputPortAsAfArray(portNum, truncateToMinLength);
}

af::array ArrayFireBlock::getInputPortAsAfArray(
    const std::string& portName,
    bool truncateToMinLength)
{
    return _getInputPortAsAfArray(portName, truncateToMinLength);
}

//
// Output port API
//

void ArrayFireBlock::postAfArray(
    size_t portNum,
    const af::array& afArray)
{
    _postAfArray(portNum, afArray);
}

void ArrayFireBlock::postAfArray(
    const std::string& portName,
    const af::array& afArray)
{
    _postAfArray(portName, afArray);
}

//
// The protected functions call into these, making the compiler generate the
// versions of these with those types.
//

template <typename PortIdType>
af::array ArrayFireBlock::_getInputPortAsAfArray(
    const PortIdType& portId,
    bool truncateToMinLength)
{
    auto bufferChunk = this->input(portId)->buffer();
    const size_t minLength = this->workInfo().minElements;
    assert(minLength <= bufferChunk.elements());

    if(truncateToMinLength && (minLength < bufferChunk.elements()))
    {
        bufferChunk.length = minLength * bufferChunk.dtype.size();
    }

    this->input(portId)->consume(minLength);
    return Pothos::Object(bufferChunk).convert<af::array>();
}

template <typename PortIdType, typename AfArrayType>
void ArrayFireBlock::_postAfArray(
    const PortIdType& portId,
    const AfArrayType& afArray)
{
    auto* outputPort = this->output(portId);
    if(outputPort->elements() < static_cast<size_t>(afArray.elements()))
    {
        throw Pothos::AssertionViolationException(
                  "Attempted to output an af::array larger than the provided buffer.",
                  Poco::format(
                      "af::array: %s elements, BufferChunk: %s elements",
                      Poco::NumberFormatter::format(afArray.elements()),
                      Poco::NumberFormatter::format(outputPort->elements())));
    }

    afArray.host(outputPort->buffer());
    outputPort->produce(afArray.elements());
}
