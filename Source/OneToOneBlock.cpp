// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BufferConversions.hpp"
#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include <cassert>
#include <cstring>
#include <string>
#include <typeinfo>

//
// Factories
//

Pothos::Block* OneToOneBlock::makeFromOneType(
    const std::string& device,
    const OneToOneFunc& func,
    const Pothos::DType& dtype,
    const DTypeSupport& supportedTypes,
    size_t numChans)
{
    validateDType(dtype, supportedTypes);

    return new OneToOneBlock(device, func, dtype, dtype, numChans);
}

Pothos::Block* OneToOneBlock::makeFloatToComplex(
    const std::string& device,
    const OneToOneFunc& func,
    const Pothos::DType& floatType,
    size_t numChans)
{
    if(!isDTypeFloat(floatType))
    {
        throw Pothos::InvalidArgumentException(
                  "This block must take a float type.",
                  "Given: " + floatType.name());
    }

    Pothos::DType complexDType("complex_"+floatType.name());

    return new OneToOneBlock(
                   device,
                   func,
                   floatType,
                   complexDType,
                   numChans);
}

Pothos::Block* OneToOneBlock::makeComplexToFloat(
    const std::string& device,
    const OneToOneFunc& func,
    const Pothos::DType& floatType,
    size_t numChans)
{
    if(!isDTypeFloat(floatType))
    {
        throw Pothos::InvalidArgumentException(
                  "This block must take a float type.",
                  "Given: " + floatType.name());
    }

    Pothos::DType complexDType("complex_"+floatType.name());

    return new OneToOneBlock(
                   device,
                   func,
                   complexDType,
                   floatType,
                   numChans);
}

//
// Class implementation
//

OneToOneBlock::OneToOneBlock(
    const std::string& device,
    const OneToOneFunc& func,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType,
    size_t numChans
): OneToOneBlock(
       device,
       Pothos::Callable(func),
       inputDType,
       outputDType,
       numChans)
{
}

OneToOneBlock::OneToOneBlock(
    const std::string& device,
    const Pothos::Callable& func,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType,
    size_t numChans
): ArrayFireBlock(device),
   _func(func),
   _nchans(numChans),
   _afOutputDType(Pothos::Object(outputDType).convert<af::dtype>())
{
    for(size_t chan = 0; chan < _nchans; ++chan)
    {
        this->setupInput(chan, inputDType);
        this->setupOutput(chan, outputDType, this->getPortDomain());
    }
}

OneToOneBlock::~OneToOneBlock() {}

af::array OneToOneBlock::getInputsAsAfArray()
{
    af::array afInput;
    if(1 == _nchans)
    {
        afInput = this->getInputPortAsAfArray(0, false);
    }
    else
    {
        afInput = getNumberedInputPortsAs2DAfArray();
        if(_nchans != static_cast<size_t>(afInput.dims(0)))
        {
            throw Pothos::AssertionViolationException(
                      "getNumberedInputPortsAs2DAfArray() returned an af::array of invalid dimensions",
                      Poco::format(
                          "Expected %s, got %s",
                          Poco::NumberFormatter::format(_nchans),
                          Poco::NumberFormatter::format(afInput.dims(0))));
        }
    }

    return afInput;
}

void OneToOneBlock::work(const af::array& afInput)
{
    const size_t elems = this->workInfo().minElements;
    if(0 == elems)
    {
        throw Pothos::AssertionViolationException(
                  "work(const af::array&) called when elems = 0",
                  __FUNCTION__);
    }

    auto afOutput = _func.call(afInput).extract<af::array>();
    if(afOutput.type() != _afOutputDType)
    {
        afOutput = afOutput.as(_afOutputDType);
    }

    this->postAfArrayToNumberedOutputPorts(afOutput);
}

// Default behavior, can be overridden
void OneToOneBlock::work()
{
#if IS_AF_CONFIG_PER_THREAD
    // The thread may have changed since the block was created, so make sure
    // the backend and device still match.
    af::setBackend(_afBackend);
    af::setDevice(_afDevice);
#endif

    const size_t elems = this->workInfo().minElements;
    if(0 == elems)
    {
        return;
    }

    this->work(getInputsAsAfArray());
}
