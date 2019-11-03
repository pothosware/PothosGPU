// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <string>

ArrayFireBlock::ArrayFireBlock():
    Pothos::Block(),
    _assumeArrayFireInputs(false),
    _afBackend(af::getActiveBackend())
{
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, getArrayFireBackend));
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, setArrayFireBackend));
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, getBlockAssumesArrayFireInputs));
    this->registerCall(this, POTHOS_FCN_TUPLE(ArrayFireBlock, setBlockAssumesArrayFireInputs));
}

ArrayFireBlock::~ArrayFireBlock()
{
}

std::string ArrayFireBlock::getArrayFireBackend() const
{
    assert(_afBackend == af::getActiveBackend());
    return Pothos::Object(_afBackend).convert<std::string>();
}

void ArrayFireBlock::setArrayFireBackend(const std::string& backend)
{
    _afBackend = Pothos::Object(backend).convert<::af_backend>();
    af::setBackend(_afBackend);
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

        // If the given array is from a different ArrayFire backend, copy
        // the contents into a new array. Otherwise, ArrayFire will throw an
        // error when performing operations on an array from a different
        // backend.
        if(af::getBackendId(*pInputAfArray) == _afBackend)
        {
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
            // In all supported versions of ArrayFire, the active backend is
            // thread-specific, so we can pull the memory out of the previous block's
            // backend and put it in the new one without affecting other blocks.
            af::setBackend(af::getBackendId(*pInputAfArray));

            // It doesn't matter what the actual underlying type is. There is
            // no void* implementation, so we need this to avoid a linker
            // error.
            std::vector<std::uint8_t> arrayCopy(pInputAfArray->bytes());
            pInputAfArray->host(arrayCopy.data());

            size_t elementSize = 0;
            auto afDType = pInputAfArray->type();
            ::af_get_size_of(&elementSize, afDType);
            assert(elementSize > 0);

            if(truncateToMinLength && (minLength < static_cast<size_t>(pInputAfArray->elements())))
            {
                arrayCopy.resize(minLength * elementSize);
            }
            const size_t numElements = arrayCopy.size() / elementSize;

            af::setBackend(_afBackend);

            ret = af::array(static_cast<dim_t>(numElements), afDType);
            ret.write<std::uint8_t>(
                arrayCopy.data(),
                numElements);
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
