// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TwoToOneBlock.hpp"
#include "Utility.hpp"

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
    const TwoToOneFunc& func,
    const Pothos::DType& dtype,
    const DTypeSupport& supportedTypes)
{
    validateDType(dtype, supportedTypes);

    return new TwoToOneBlock(func, dtype, dtype);
}

Pothos::Block* TwoToOneBlock::makeFromTwoTypes(
    const TwoToOneFunc& func,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType,
    const DTypeSupport& supportedInputTypes,
    const DTypeSupport& supportedOutputTypes)
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

    return new TwoToOneBlock(func, inputDType, outputDType);
}

//
// Class implementation
//

TwoToOneBlock::TwoToOneBlock(
    const TwoToOneFunc& func,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType
): ArrayFireBlock(),
   _func(func)
{
    this->setupInput(0, inputDType);
    this->setupInput(1, inputDType);
    this->setupOutput(0, outputDType);
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

    assert(elems == inputAfArray0.elements());
    assert(elems == inputAfArray1.elements());

    auto outputAfArray = _func(inputAfArray0, inputAfArray1);

    this->input(0)->consume(elems);
    this->input(1)->consume(elems);
    this->postAfArray(0, outputAfArray);
}
