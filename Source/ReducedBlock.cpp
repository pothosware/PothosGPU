// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ReducedBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cassert>
#include <string>
#include <typeinfo>

using ReducedFunc = af::array(*)(const af::array, const int);

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
        this->setupInput(chan, inputDType, this->getPortDomain());
    }
    this->setupOutput(0, outputDType, this->getPortDomain());
}

ReducedBlock::~ReducedBlock() {}

void ReducedBlock::work()
{
    const size_t elems = this->workInfo().minAllInElements;

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

    this->postAfArray(0, std::move(afOutput));
}
