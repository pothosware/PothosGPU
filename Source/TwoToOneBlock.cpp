// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TwoToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cassert>
#include <string>
#include <typeinfo>

//
// Factories
//

Pothos::Block* TwoToOneBlock::makeFromOneType(
    const std::string& device,
    const TwoToOneFunc& func,
    const Pothos::DType& dtype,
    const DTypeSupport& supportedTypes,
    bool allowZeroInBuffer1)
{
    validateDType(dtype, supportedTypes);

    return new TwoToOneBlock(
                   device,
                   func,
                   dtype,
                   dtype,
                   allowZeroInBuffer1);
}

Pothos::Block* TwoToOneBlock::makeFloatToComplex(
    const std::string& device,
    const TwoToOneFunc& func,
    const Pothos::DType& floatType,
    bool allowZeroInBuffer1)
{
    if(!isDTypeFloat(floatType))
    {
        throw Pothos::InvalidArgumentException(
                  "This block must take a float type.",
                  "Given: " + floatType.name());
    }

    Pothos::DType complexDType("complex_"+floatType.name());

    return new TwoToOneBlock(
                   device,
                   func,
                   floatType,
                   complexDType,
                   allowZeroInBuffer1);
}

Pothos::Block* TwoToOneBlock::makeComparator(
    const std::string& device,
    const TwoToOneFunc& func,
    const Pothos::DType& dtype,
    const DTypeSupport& supportedTypes)
{
    validateDType(dtype, supportedTypes);

    static const Pothos::DType Int8DType("int8");

    return new TwoToOneBlock(
                   device,
                   func,
                   dtype,
                   Int8DType,
                   true /*allowZeroInBuffer1*/);
}

//
// Class implementation
//

TwoToOneBlock::TwoToOneBlock(
    const std::string& device,
    const TwoToOneFunc& func,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType,
    bool allowZeroInBuffer1
): ArrayFireBlock(device),
   _func(func),
   _allowZeroInBuffer1(allowZeroInBuffer1)
{
    this->setupInput(0, inputDType, _domain);
    this->setupInput(1, inputDType, _domain);
    this->setupOutput(0, outputDType, _domain);
}

TwoToOneBlock::~TwoToOneBlock() {}

void TwoToOneBlock::work()
{
    const size_t elems = this->workInfo().minAllElements;
    if(0 == elems)
    {
        return;
    }

    auto inputAfArray0 = this->getInputPortAsAfArray(0);
    auto inputAfArray1 = this->getInputPortAsAfArray(1);

    if(!_allowZeroInBuffer1 && (elems != static_cast<size_t>(inputAfArray1.nonzeros())))
    {
        throw Pothos::InvalidArgumentException("Denominator cannot contain zeros.");
    }

    auto outputAfArray = _func(inputAfArray0, inputAfArray1);
    this->produceFromAfArray(0, outputAfArray);
}
