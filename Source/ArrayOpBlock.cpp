// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "NToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cassert>
#include <cstring>
#include <string>
#include <typeinfo>

enum class ArrayOpBlockType
{
    ARITHMETIC,
    COMPARATOR,
    BITWISE,
    LOGICAL
};

#define IfTypeThenLambda(op, callerOp, dest) \
    if(#op == callerOp) \
        dest = [](const af::array& a, const af::array& b){return a op b;};

// TODO: TwoToOne: comparators, << >>

static Pothos::Block* makeArrayOpBlock(
    ArrayOpBlockType blockType,
    const std::string& device,
    const std::string& operation,
    const Pothos::DType& dtype,
    size_t numChans)
{
    NToOneFunc func = nullptr;

    // TODO: see what fails
    DTypeSupport dtypeSupport = {true,true,true,true};

    switch(blockType)
    {
        case ArrayOpBlockType::ARITHMETIC:
            IfTypeThenLambda(+, operation, func)
            IfTypeThenLambda(-, operation, func)
            IfTypeThenLambda(*, operation, func)
            IfTypeThenLambda(/, operation, func)
            else throw Pothos::InvalidArgumentException("Invalid operation", operation);
            break;

        case ArrayOpBlockType::BITWISE:
            IfTypeThenLambda(&, operation, func)
            else IfTypeThenLambda(|, operation, func)
            else IfTypeThenLambda(^, operation, func)
            else throw Pothos::InvalidArgumentException("Invalid operation", operation);
            break;

        case ArrayOpBlockType::LOGICAL:
            IfTypeThenLambda(&&, operation, func)
            else IfTypeThenLambda(||, operation, func)
            else throw Pothos::InvalidArgumentException("Invalid operation", operation);
            break;

        default:
            throw Pothos::AssertionViolationException("Invalid ArrayOpBlockType");
    }
    if(nullptr == func) throw Pothos::AssertionViolationException("nullptr == func");

    return NToOneBlock::make(
               device,
               func,
               dtype,
               numChans,
               dtypeSupport);
}

static Pothos::BlockRegistry registerArrayArithmetic(
    "/arrayfire/array/arithmetic",
    Pothos::Callable(&makeArrayOpBlock).bind(ArrayOpBlockType::ARITHMETIC, 0));

/*
static Pothos::BlockRegistry registerArrayComparator(
    "/arrayfire/array/comparator",
    Pothos::Callable(&makeArrayOpBlock).bind(ArrayOpBlockType::COMPARATOR, 0));
*/

static Pothos::BlockRegistry registerArrayBitwise(
    "/arrayfire/array/bitwise",
    Pothos::Callable(&makeArrayOpBlock).bind(ArrayOpBlockType::BITWISE, 0));

static Pothos::BlockRegistry registerArrayLogical(
    "/arrayfire/array/logical",
    Pothos::Callable(&makeArrayOpBlock).bind(ArrayOpBlockType::LOGICAL, 0));
