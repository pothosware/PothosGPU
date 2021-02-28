// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ReducedBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include <arrayfire.h>

#include <cassert>
#include <string>
#include <typeinfo>

namespace PothosGPU
{

ReducedBlock::ReducedBlock(
    const std::string& device,
    const ReducedFunc& func,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType,
    size_t numChannels
):
    ArrayFireBlock(device),
    _func(func),
    _afOutputDType(Pothos::Object(outputDType).convert<af::dtype>()),
    _nchans(numChannels)
{
    // Input validation
    if(numChannels < 2)
    {
        throw Pothos::InvalidArgumentException("numChannels must be >= 2.");
    }

    // Set up ports
    for(size_t chan = 0; chan < _nchans; ++chan)
    {
        this->setupInput(chan, inputDType, _domain);
    }
    this->setupOutput(0, outputDType, _domain);
}

ReducedBlock::~ReducedBlock() {}

af::array ReducedBlock::getNumberedInputPortsAs2DAfArray()
{
    // Assumptions:
    //  * We've already checked that all buffers are non-empty.
    //  * We only have numbered ports.
    //  * All DTypes are the same.
    const auto& inputs = this->inputs();

    const auto dim0 = static_cast<dim_t>(inputs.size());
    const auto dim1 = static_cast<dim_t>(this->workInfo().minElements);
    const auto afDType = Pothos::Object(inputs[0]->dtype()).convert<af::dtype>();

    af::array ret(dim0, dim1, afDType);
    for(dim_t row = 0; row < dim0; ++row)
    {
        auto afArray = this->getInputPortAsAfArray(row);
        if(afArray.elements() != dim1)
        {
            throw Pothos::AssertionViolationException(
                      "getInputPortAsAfArray() returned an af::array of invalid size",
                      Poco::format(
                          "Expected %s, got %s",
                          Poco::NumberFormatter::format(dim1),
                          Poco::NumberFormatter::format(afArray.elements())));
        }

        ret.row(row) = afArray;
    }

    return ret;
}

void ReducedBlock::work()
{
    const size_t elems = this->workInfo().minAllElements;

    if(0 == elems)
    {
        return;
    }

    auto afArray = this->getNumberedInputPortsAs2DAfArray();
    auto afOutput = _func(afArray, -1).as(_afOutputDType);

    if(elems != static_cast<size_t>(afOutput.elements()))
    {
        throw Pothos::AssertionViolationException(
                  "Unexpected output size",
                  std::to_string(elems));
    }

    this->produceFromAfArray(0, afOutput);
}

}
