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
    const DTypeSupport& supportedTypes)
{
    validateDType(dtype, supportedTypes);

    return new OneToOneBlock(device, func, dtype, dtype);
}

Pothos::Block* OneToOneBlock::makeFromOneTypeCallable(
    const std::string& device,
    const Pothos::Callable& func,
    const Pothos::DType& dtype,
    const DTypeSupport& supportedTypes)
{
    validateDType(dtype, supportedTypes);

    return new OneToOneBlock(device, func, dtype, dtype);
}

Pothos::Block* OneToOneBlock::makeFloatToComplex(
    const std::string& device,
    const OneToOneFunc& func,
    const Pothos::DType& floatType)
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
                   complexDType);
}

Pothos::Block* OneToOneBlock::makeComplexToFloat(
    const std::string& device,
    const OneToOneFunc& func,
    const Pothos::DType& floatType)
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
                   floatType);
}

//
// Class implementation
//

OneToOneBlock::OneToOneBlock(
    const std::string& device,
    const OneToOneFunc& func,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType
): OneToOneBlock(
       device,
       Pothos::Callable(func),
       inputDType,
       outputDType)
{
}

OneToOneBlock::OneToOneBlock(
    const std::string& device,
    const Pothos::Callable& func,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType
): ArrayFireBlock(device),
   _func(func),
   _afOutputDType(Pothos::Object(outputDType).convert<af::dtype>())
{
    this->setupInput(0, inputDType);
    this->setupOutput(0, outputDType);
}

OneToOneBlock::~OneToOneBlock() {}

// Default behavior, can be overridden
void OneToOneBlock::work()
{
    // The thread may have changed since the block was created, so make sure
    // the backend and device still match.
    af::setBackend(_afBackend);
    af::setDevice(_afDevice);

    const size_t elems = this->workInfo().minElements;
    if(0 == elems)
    {
        return;
    }

    auto afInput = this->getInputPortAsAfArray(0);

    auto afOutput = _func.call(afInput).extract<af::array>();
    if(afOutput.type() != _afOutputDType)
    {
        afOutput = afOutput.as(_afOutputDType);
    }

    this->produceFromAfArray(0, afOutput);
}
