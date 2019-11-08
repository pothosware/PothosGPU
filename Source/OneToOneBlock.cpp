// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cassert>
#include <cstring>
#include <string>
#include <typeinfo>

//
// Factories
//

Pothos::Block* OneToOneBlock::makeFromOneType(
    const OneToOneFunc& func,
    const Pothos::DType& dtype,
    const DTypeSupport& supportedTypes,
    size_t numChans)
{
    validateDType(dtype, supportedTypes);

    return new OneToOneBlock(func, dtype, dtype, numChans);
}

Pothos::Block* OneToOneBlock::makeFromTwoTypes(
    const OneToOneFunc& func,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType,
    const DTypeSupport& supportedInputTypes,
    const DTypeSupport& supportedOutputTypes,
    size_t numChans)
{
    validateDType(inputDType, supportedInputTypes);
    validateDType(outputDType, supportedOutputTypes);

    if(isDTypeComplexFloat(inputDType) && isDTypeFloat(outputDType))
    {
        validateComplexAndFloatTypesMatch(
            inputDType,
            outputDType);
    }
    else if(isDTypeFloat(inputDType) && isDTypeComplexFloat(outputDType))
    {
        validateComplexAndFloatTypesMatch(
            outputDType,
            inputDType);
    }

    return new OneToOneBlock(func, inputDType, outputDType, numChans);
}

//
// Class implementation
//

OneToOneBlock::OneToOneBlock(
    const OneToOneFunc& func,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType,
    size_t numChans
): OneToOneBlock(
       Pothos::Callable(func),
       inputDType,
       outputDType,
       numChans)
{
}

OneToOneBlock::OneToOneBlock(
    const Pothos::Callable& func,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType,
    size_t numChans
): ArrayFireBlock(),
   _func(func),
   _nchans(numChans),
   _afOutputDType(Pothos::Object(outputDType).convert<af::dtype>())
{
    for(size_t chan = 0; chan < _nchans; ++chan)
    {
        this->setupInput(chan, inputDType);
        this->setupOutput(chan, outputDType);
    }
}

OneToOneBlock::~OneToOneBlock() {}

af::array OneToOneBlock::getInputsAsAfArray()
{
// This variable is only used in debug.
#ifndef NDEBUG
    const size_t elems = this->workInfo().minElements;
#endif
    assert(this->workInfo().minElements > 0);

    af::array afInput;
    if(1 == _nchans)
    {
        afInput = this->getInputPortAsAfArray(0, false);
        assert(elems == static_cast<size_t>(afInput.elements()));
    }
    else
    {
        assert(0 != _nchans);

        afInput = getNumberedInputPortsAs2DAfArray();
        assert(_nchans == static_cast<size_t>(afInput.dims(0)));
        assert(elems == static_cast<size_t>(afInput.dims(1)));
    }

    return afInput;
}

void OneToOneBlock::post2DAfArrayToNumberedOutputPorts(const af::array& afArray)
{
    const auto& outputs = this->outputs();
    assert(outputs.size() == static_cast<size_t>(afArray.dims(0)));

    for(size_t portIndex = 0; portIndex < outputs.size(); ++portIndex)
    {
        auto outputBuffer = Pothos::Object(afArray.row(portIndex))
                                .convert<Pothos::BufferChunk>();
        outputs[portIndex]->postBuffer(std::move(outputBuffer));
    }
}

void OneToOneBlock::work(const af::array& afInput)
{
    const size_t elems = this->workInfo().minElements;
    assert(elems > 0);

    if(1 == _nchans)
    {
        auto afOutput = _func.call(afInput).extract<af::array>();
        if(afOutput.type() != _afOutputDType)
        {
            afOutput = afOutput.as(_afOutputDType);
        }

        this->input(0)->consume(elems);
        this->output(0)->postBuffer(Pothos::Object(afOutput)
                                        .convert<Pothos::BufferChunk>());
    }
    else
    {
        assert(0 != _nchans);
        assert(_nchans == static_cast<size_t>(afInput.dims(0)));
        assert(elems == static_cast<size_t>(afInput.dims(1)));

        auto afOutput = _func.call(afInput).extract<af::array>();
        if(afOutput.type() != _afOutputDType)
        {
            afOutput = afOutput.as(_afOutputDType);
        }

        post2DAfArrayToNumberedOutputPorts(afOutput);
    }
}

// Default behavior, can be overridden
void OneToOneBlock::work()
{
    const size_t elems = this->workInfo().minElements;
    if(0 == elems)
    {
        return;
    }

    work(getInputsAsAfArray());
}
