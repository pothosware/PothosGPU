// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <string>

ArrayFireBlock::ArrayFireBlock(): Pothos::Block()
{
}

ArrayFireBlock::~ArrayFireBlock()
{
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

template <typename T>
af::array ArrayFireBlock::_getInputPortAsAfArray(
    const T& portId,
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

template <typename T>
void ArrayFireBlock::_postAfArray(
    const T& portId,
    const af::array& afArray)
{
    auto bufferChunk = Pothos::Object(afArray).convert<Pothos::BufferChunk>();
    this->output(portId)->postBuffer(bufferChunk);
}
