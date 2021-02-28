// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "BufferConversions.hpp"
#include "DeviceCache.hpp"
#include "SharedBufferAllocator.hpp"
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

#ifdef POTHOSGPU_LEGACY_BUFFER_MANAGER
Pothos::BufferManager::Sptr makePinnedBufferManager(af::Backend backend);
#endif

static void checkVersion()
{
    static constexpr size_t buildAPIVersion = AF_API_VERSION_CURRENT;

    int runtimeMajor, runtimeMinor, runtimePatch;
    ::af_get_version(&runtimeMajor, &runtimeMinor, &runtimePatch);

    const size_t runtimeAPIVersion = static_cast<size_t>((runtimeMajor*10) + runtimeMinor);

    if(buildAPIVersion != runtimeAPIVersion)
    {
        throw Pothos::Exception(
                  "Incompatible buildtime and runtime ArrayFire versions",
                  Poco::format(
                      "Buildtime=%s, Runtime=%d.%d.%d",
                      std::string(AF_VERSION),
                      runtimeMajor,
                      runtimeMinor,
                      runtimePatch));
    }
}

ArrayFireBlock::ArrayFireBlock(const std::string& device):
    Pothos::Block(),
    _afDeviceName(device)
{
    checkVersion();

    const auto& deviceCache = getDeviceCache();
    if(deviceCache.empty())
    {
        throw Pothos::RuntimeException("No ArrayFire devices found. Check your ArrayFire installation.");
    }

    if(device == "Auto")
    {
        _afBackend = deviceCache[0].afBackendEnum;
        _afDevice = deviceCache[0].afDeviceIndex;
        _afDeviceName = deviceCache[0].name;
    }
    else
    {
        auto deviceCacheIter = std::find_if(
                                   deviceCache.begin(),
                                   deviceCache.end(),
                                   [&device](const DeviceCacheEntry& entry)
                                   {
                                       return (entry.name == device) ||
                                              (Poco::format("%s:%d", entry.platform, entry.afDeviceIndex) == device);
                                   });
        if(deviceCache.end() != deviceCacheIter)
        {
            _afBackend = deviceCacheIter->afBackendEnum;
            _afDevice = deviceCacheIter->afDeviceIndex;
            _afDeviceName = deviceCacheIter->name;
        }
        else
        {
            throw Pothos::InvalidArgumentException(
                      Poco::format(
                          "Could not find ArrayFire device %s.",
                          device));
        }
    }

    _domain = "ArrayFire_" + this->backend();

    this->configArrayFire();
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, backend));
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, device));
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, overlay));
}

ArrayFireBlock::~ArrayFireBlock()
{
}

Pothos::BufferManager::Sptr ArrayFireBlock::getInputBufferManager(
    const std::string& /*name*/,
    const std::string& domain)
{
    if(domain.empty())
    {
        Pothos::BufferManager::Sptr bufferManager;
#ifdef POTHOSGPU_LEGACY_BUFFER_MANAGER
        bufferManager = makePinnedBufferManager(_afBackend);
#else
        bufferManager = Pothos::BufferManager::make("generic");
        bufferManager->setAllocateFunction(getSharedBufferAllocator(_afBackend));
#endif
        return bufferManager;
    }
    else if(domain == _domain) return Pothos::BufferManager::Sptr();
    else throw Pothos::PortDomainError(domain);
}

Pothos::BufferManager::Sptr ArrayFireBlock::getOutputBufferManager(
    const std::string& /*name*/,
    const std::string& domain)
{
    if(domain.empty() || (domain == _domain))
    {
        Pothos::BufferManager::Sptr bufferManager;
#ifdef POTHOSGPU_LEGACY_BUFFER_MANAGER
        bufferManager = makePinnedBufferManager(_afBackend);
#else
        bufferManager = Pothos::BufferManager::make("generic");
        bufferManager->setAllocateFunction(getSharedBufferAllocator(_afBackend));
#endif
        return bufferManager;
    }
    else throw Pothos::PortDomainError(domain);
}

void ArrayFireBlock::activate()
{
    this->configArrayFire();
}

std::string ArrayFireBlock::backend() const
{
    return Pothos::Object(_afBackend).convert<std::string>();
}

std::string ArrayFireBlock::device() const
{
    return _afDeviceName;
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
        option["name"] = entry.name;
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

void ArrayFireBlock::produceFromAfArray(
    size_t portNum,
    const af::array& afArray)
{
    _produceFromAfArray(portNum, afArray);
}

void ArrayFireBlock::produceFromAfArray(
    const std::string& portName,
    const af::array& afArray)
{
    _produceFromAfArray(portName, afArray);
}

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
// Misc
//

void ArrayFireBlock::configArrayFire() const
{
    if(af::getActiveBackend() != _afBackend)
    {
        af::setBackend(_afBackend);
    }
    if(af::getDevice() != _afDevice)
    {
        af::setDevice(_afDevice);
    }
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
    const size_t minLength = this->workInfo().minAllElements;
    assert(minLength <= bufferChunk.elements());

    if(truncateToMinLength && (minLength < bufferChunk.elements()))
    {
        bufferChunk.length = minLength * bufferChunk.dtype.size();
    }

    this->input(portId)->consume(minLength);
    return Pothos::Object(bufferChunk).convert<af::array>();
}

template <typename PortIdType, typename AfArrayType>
void ArrayFireBlock::_produceFromAfArray(
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
    else if(afArray.elements() == 0)
    {
        throw Pothos::AssertionViolationException(
                "Attempted to output an empty af::array,",
                "Port: "+Pothos::Object(portId).convert<std::string>());
    }

    afArray.host(outputPort->buffer());
    outputPort->produce(afArray.elements());
}

template <typename PortIdType, typename AfArrayType>
void ArrayFireBlock::_postAfArray(
    const PortIdType& portId,
    const AfArrayType& afArray)
{
    if(afArray.elements() == 0)
    {
        throw Pothos::AssertionViolationException(
                "Attempted to output an empty af::array,",
                "Port: "+Pothos::Object(portId).convert<std::string>());
    }
    this->output(portId)->postBuffer(Pothos::Object(afArray).convert<Pothos::BufferChunk>());
}
