// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "NToOneBlock.hpp"
#include "ReducedBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <string>
#include <typeinfo>
#include <unordered_map>

#define NToOneLambda(op) \
    [](const af::array& a, const af::array& b){return a op b;}

#define IfOpThenReducedBlock(op, func) \
    if(#op == operation) \
        return new ReducedBlock(device, &func, dtype, dtype, numChans);

#define IfOpThenNToOneBlock(op) \
    if(#op == operation) \
        return new NToOneBlock(device, NToOneLambda(op), dtype, numChans);

static Pothos::Block* makeArrayArithmetic(
    const std::string& device,
    const std::string& operation,
    const Pothos::DType& dtype,
    size_t numChans)
{
    IfOpThenReducedBlock(+, af::sum)
    else IfOpThenNToOneBlock(-)
    else IfOpThenReducedBlock(*, af::product)
    else IfOpThenNToOneBlock(/)

    throw Pothos::InvalidArgumentException("Invalid operation", operation);
}

static Pothos::Block* makeArrayBitwise(
    const std::string& device,
    const std::string& operation,
    const Pothos::DType& dtype,
    size_t numChans)
{
    IfOpThenNToOneBlock(&)
    else IfOpThenNToOneBlock(|)
    else IfOpThenNToOneBlock(^)

    throw Pothos::InvalidArgumentException("Invalid operation", operation);
}

static Pothos::Block* makeArrayLogical(
    const std::string& device,
    const std::string& operation,
    const Pothos::DType& dtype,
    size_t numChans)
{
    IfOpThenReducedBlock(&&, af::allTrue)
    else IfOpThenReducedBlock(||, af::anyTrue)

    throw Pothos::InvalidArgumentException("Invalid operation", operation);
}

// TODO: TwoToOne: comparators, << >>

static Pothos::BlockRegistry registerArrayArithmetic(
    "/arrayfire/array/arithmetic",
    Pothos::Callable(&makeArrayArithmetic));

static Pothos::BlockRegistry registerArrayBitwise(
    "/arrayfire/array/bitwise",
    Pothos::Callable(&makeArrayBitwise));

static Pothos::BlockRegistry registerArrayLogical(
    "/arrayfire/array/logical",
    Pothos::Callable(&makeArrayLogical));
