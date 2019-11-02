// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <string>

ArrayFireBlock::ArrayFireBlock():
    Pothos::Block(),
    _assumeArrayFireInputs(false)
{
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, getBlockAssumesArrayFireInputs));
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, setBlockAssumesArrayFireInputs));
}

ArrayFireBlock::~ArrayFireBlock()
{
}

bool ArrayFireBlock::getBlockAssumesArrayFireInputs() const
{
    return _assumeArrayFireInputs;
}

void ArrayFireBlock::setBlockAssumesArrayFireInputs(bool value)
{
    _assumeArrayFireInputs = value;
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

    af::array ret;

    if(_assumeArrayFireInputs)
    {
        auto sharedBuffer = bufferChunk.getBuffer();
        auto* pInputAfArray = reinterpret_cast<af::array*>(sharedBuffer.getContainer().get());
        assert(nullptr != pInputAfArray);

        if(truncateToMinLength && (minLength < bufferChunk.elements()))
        {
            ret = pInputAfArray->slice(static_cast<int>(minLength));
        }
        else
        {
            ret = *pInputAfArray;
        }
    }
    else
    {
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

        ret = Pothos::Object(bufferChunk).convert<af::array>();
    }

    return ret;
}

template <typename T>
void ArrayFireBlock::_postAfArray(
    const T& portId,
    const af::array& afArray)
{
    auto bufferChunk = Pothos::Object(afArray).convert<Pothos::BufferChunk>();
    this->output(portId)->postBuffer(bufferChunk);
}
