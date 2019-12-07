// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "DeviceCache.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>

#include <arrayfire.h>

#include <algorithm>
#include <string>

// TODO: make sure device supports double
ArrayFireBlock::ArrayFireBlock():
    Pothos::Block(),
    _afBackend(af::getActiveBackend()),
    _afDevice(af::getDevice())
{
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, getArrayFireBackend));
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, setArrayFireBackend));
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, getArrayFireDevice));
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, setArrayFireDevice));
}

ArrayFireBlock::~ArrayFireBlock()
{
}

std::string ArrayFireBlock::getArrayFireBackend() const
{
    assert(_afBackend == af::getActiveBackend());
    return Pothos::Object(_afBackend).convert<std::string>();
}

void ArrayFireBlock::setArrayFireBackend(const Pothos::Object& backend)
{
    if(!this->isActive())
    {
        auto afBackend = backend.convert<af::Backend>();
        setThreadAFBackend(afBackend);

        _afBackend = afBackend;

        // New backend likely means new device ID.
        _afDevice = af::getDevice();
    }
    else
    {
        throw Pothos::RuntimeException("Cannot change a block's backend while the block is active.");
    }
}

std::string ArrayFireBlock::getArrayFireDevice() const
{
    const auto& deviceCache = getDeviceCache();
    auto deviceIter = std::find_if(
                          deviceCache.begin(),
                          deviceCache.end(),
                          [this](const DeviceCacheEntry& entry)
                          {
                              return (entry.afBackendEnum == this->_afBackend) &&
                                     (entry.afDeviceIndex == this->_afDevice);
                          });
    assert(deviceIter != deviceCache.end());

    return deviceIter->name;
}

void ArrayFireBlock::setArrayFireDevice(const std::string& device)
{
    if(!this->isActive())
    {
        const auto& deviceCache = getDeviceCache();
        auto deviceIter = std::find_if(
                              deviceCache.begin(),
                              deviceCache.end(),
                              [this, &device](const DeviceCacheEntry& entry)
                              {
                                  return (entry.afBackendEnum == this->_afBackend) &&
                                         (entry.name == device);
                              });
        if(deviceIter != deviceCache.end())
        {
            setThreadAFDevice(device);

            _afDevice = deviceIter->afDeviceIndex;
        }
        else
        {
            throw Pothos::InvalidArgumentException(
                      Poco::format(
                          "Could not find %s device with name \"%s\"",
                          Pothos::Object(_afBackend).convert<std::string>(),
                          device));
        }
    }
    else
    {
        throw Pothos::RuntimeException("Cannot change a block's device while the block is active.");
    }
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
// The underlying implementation for moving buffers back and forth between
// Pothos and ArrayFire.
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
        auto sharedBuffer = bufferChunk.getBuffer();
        auto dtype = bufferChunk.dtype;

        auto newSharedBuffer = Pothos::SharedBuffer(
                                   sharedBuffer.getAddress(),
                                   minLength * dtype.size(),
                                   sharedBuffer);

        bufferChunk = Pothos::BufferChunk(newSharedBuffer);
        bufferChunk.dtype = dtype;
    }

    return Pothos::Object(bufferChunk).convert<af::array>();
}

template <typename PortIdType, typename AfArrayType>
void ArrayFireBlock::_postAfArray(
    const PortIdType& portId,
    const AfArrayType& afArray)
{
    auto bufferChunk = Pothos::Object(afArray).convert<Pothos::BufferChunk>();
    this->output(portId)->postBuffer(bufferChunk);
}
