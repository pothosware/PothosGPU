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
): ArrayFireBlock(),
   _func(func),
   _nchans(numChans)
{
    for(size_t chan = 0; chan < _nchans; ++chan)
    {
        this->setupInput(chan, inputDType);
        this->setupOutput(chan, outputDType);
    }
}

OneToOneBlock::~OneToOneBlock() {}

af::array OneToOneBlock::getNumberedInputPortsAs2DAfArray()
{
    // Assumptions:
    //  * We've already checked that all buffers are non-empty.
    //  * We only have numbered ports.
    //  * All DTypes are the same.
    const auto& inputs = this->inputs();
    assert(!inputs.empty());

    const auto dim0 = static_cast<dim_t>(inputs.size());
    const auto dim1 = static_cast<dim_t>(this->workInfo().minElements);
    const auto afDType = Pothos::Object(inputs[0]->dtype()).convert<::af_dtype>();
    const size_t rowSizeBytes = inputs[0]->dtype().size() * dim1;

    af::array ret(dim0, dim1, afDType);
    for(dim_t row = 0; row < dim0; ++row)
    {
        auto arrayRow = ret.row(row);
        std::memcpy(
            arrayRow.device<std::uint8_t>(),
            inputs[row]->buffer().as<const std::uint8_t*>(),
            rowSizeBytes);
        arrayRow.unlock();

        inputs[row]->consume(dim1);
    }

    return ret;
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

void OneToOneBlock::work()
{
    const size_t elems = this->workInfo().minElements;
    if(0 == elems)
    {
        return;
    }

    if(1 == _nchans)
    {
        auto afInput = Pothos::Object(this->input(0)->buffer())
                           .convert<af::array>();
        auto afOutput = _func(afInput);

        this->input(0)->consume(elems);
        this->output(0)->postBuffer(Pothos::Object(afOutput)
                                        .convert<Pothos::BufferChunk>());
    }
    else
    {
        assert(0 != _nchans);

        auto afInput = getNumberedInputPortsAs2DAfArray();
        const auto dim0 = afInput.dims(0);
        const auto dim1 = afInput.dims(1);

        assert(_nchans == static_cast<size_t>(dim0));
        assert(elems == static_cast<size_t>(dim1));

        af::array afOutput(dim0, dim1, afInput.type());
        gfor(af::seq row, dim0)
        {
            afOutput(row) = _func(afInput(row));
        }

        post2DAfArrayToNumberedOutputPorts(afOutput);
    }
}
