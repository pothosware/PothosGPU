// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "SingleOutputSource.hpp"
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

Pothos::Block* SingleOutputSource::make(
    const SingleOutputFunc& func,
    const Pothos::DType& dtype,
    const DTypeSupport& supportedTypes)
{
    validateDType(dtype, supportedTypes);

    return new SingleOutputSource(func, dtype);
}

//
// Class implementation
//

SingleOutputSource::SingleOutputSource(
    const SingleOutputFunc& func,
    const Pothos::DType& dtype
): ArrayFireBlock(),
   _func(func),
   _afDType(Pothos::Object(dtype).convert<af::dtype>())
{
    this->setupOutput(0, dtype);
}

SingleOutputSource::~SingleOutputSource() {}

void SingleOutputSource::work()
{
    this->postAfArray(
        0,
        _func(OutputBufferSize, _afDType));
}
