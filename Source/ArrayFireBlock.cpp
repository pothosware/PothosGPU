// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "DeviceCache.hpp"
#include "Utility.hpp"

// TODO: move stdVectorToString to common debug header, don't depend on test
#include "Testing/TestUtility.hpp"

#include <nlohmann/json.hpp>

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>
#include <Poco/Logger.h>

#include <arrayfire.h>

#include <algorithm>
#include <string>

// TODO: overlay

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
    defaultOption["value"] = "Auto";
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

bool ArrayFireBlock::doesInputPortDomainMatch(size_t portNum) const
{
    return _doesInputPortDomainMatch(portNum);
}

bool ArrayFireBlock::doesInputPortDomainMatch(const std::string& portName) const
{
    return _doesInputPortDomainMatch(portName);
}

const af::array& ArrayFireBlock::getInputPortAfArrayRef(size_t portNum)
{
    return _getInputPortAfArrayRef(portNum);
}

const af::array& ArrayFireBlock::getInputPortAfArrayRef(const std::string& portName)
{
    return _getInputPortAfArrayRef(portName);
}

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

af::array ArrayFireBlock::getNumberedInputPortsAs2DAfArray()
{
    // Assumptions:
    //  * We've already checked that all buffers are non-empty.
    //  * We only have numbered ports.
    //  * All DTypes are the same.
    const auto& inputs = this->inputs();
    assert(!inputs.empty());

    const auto dim0 = static_cast<dim_t>(inputs.size());
    const auto dim1 = static_cast<dim_t>(this->workInfo().minElements);
    const auto afDType = Pothos::Object(inputs[0]->dtype()).convert<af::dtype>();

    af::array ret(dim0, dim1, afDType);
    for(dim_t row = 0; row < dim0; ++row)
    {
        ret.row(row) = this->getInputPortAsAfArray(row);
        assert(ret.row(row).elements() == dim1);

        inputs[row]->consume(dim1);
    }

    return ret;
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

void ArrayFireBlock::post2DAfArrayToNumberedOutputPorts(const af::array& afArray)
{
    const size_t numOutputs = this->outputs().size();
    assert(numOutputs == static_cast<size_t>(afArray.dims(0)));

    for(size_t portIndex = 0; portIndex < numOutputs; ++portIndex)
    {
        this->postAfArray(portIndex, afArray.row(portIndex));
    }
}

//
// Debug
//

void ArrayFireBlock::debugLogInputPortElements()
{
#ifndef NDEBUG
    std::vector<int> elements;
    std::transform(
        this->inputs().begin(),
        this->inputs().end(),
        std::back_inserter(elements),
        [](Pothos::InputPort* port){return port->elements();});

    poco_information(
        Poco::Logger::get(this->getName()),
        stdVectorToString(elements));
#endif
}

//
// The protected functions call into these, making the compiler generate the
// versions of these with those types.
//

template <typename PortIdType>
bool ArrayFireBlock::_doesInputPortDomainMatch(const PortIdType& portId) const
{
    return (this->input(portId)->domain() == this->getPortDomain());
}

template <typename PortIdType>
const af::array& ArrayFireBlock::_getInputPortAfArrayRef(const PortIdType& portId)
{
    if(!_doesInputPortDomainMatch(portId))
    {
        throw Pothos::AssertionViolationException("Called _getInputPortAfArrayRef() with invalid port");
    }

    const auto& containerSPtr = this->input(portId)->buffer().getBuffer().getContainer();
    return *reinterpret_cast<af::array*>(containerSPtr.get());
}

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
        bufferChunk.length = minLength * bufferChunk.dtype.elemSize();
    }

    return Pothos::Object(bufferChunk).convert<af::array>();
}

template <typename PortIdType, typename AfArrayType>
void ArrayFireBlock::_postAfArray(
    const PortIdType& portId,
    const AfArrayType& afArray)
{
    auto bufferChunk = Pothos::Object(afArray).convert<Pothos::BufferChunk>();
    this->output(portId)->postBuffer(std::move(bufferChunk));
}
