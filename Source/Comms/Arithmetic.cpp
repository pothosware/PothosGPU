// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "NToOneBlock.hpp"
#include "ReducedBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <string>

#define NToOneLambda(op) \
    [](const af::array& a, const af::array& b){return a op b;}

#define IfOpThenReducedBlock(str, func) \
    if(#str == operation) \
        return new ReducedBlock(device, &func, dtype, dtype, numInputs);

#define IfOpThenNToOneBlock(str, op) \
    if(#str == operation) \
        return new NToOneBlock(device, NToOneLambda(op), dtype, numInputs);

static Pothos::Block* makeCommsArithmetic(
    const std::string& device,
    const Pothos::DType& dtype,
    const std::string& operation,
    size_t numInputs)
{
    IfOpThenReducedBlock(ADD, af::sum)
    else IfOpThenNToOneBlock(SUB, -)
    else IfOpThenReducedBlock(MUL, af::product)
    else IfOpThenNToOneBlock(DIV, /)

    throw Pothos::InvalidArgumentException("Invalid operation", operation);
}

/***********************************************************************
 * |PothosDoc Arithmetic (Parallel)
 *
 * Perform arithmetic operations on elements across multiple input ports to produce a stream of outputs.
 *
 * out[n] = in0[n] $op in1[n] $op ... $op in_last[n]
 *
 * |category /ArrayFire/Comms
 * |keywords math arithmetic add subtract multiply divide
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 * |widget ComboBox(editable=false)
 * |preview enable
 *
 * |param dtype[Data Type] The data type used in the arithmetic.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1,dim=1)
 * |default "complex_float32"
 * |preview disable
 *
 * |param operation The mathematical operation to perform
 * |default "ADD"
 * |option [Add] "ADD"
 * |option [Subtract] "SUB"
 * |option [Multiply] "MUL"
 * |option [Divide] "DIV"
 *
 * |param numInputs[Num Inputs] The number of input ports.
 * |default 2
 * |widget SpinBox(minimum=2)
 * |preview disable
 *
 * |factory /arrayfire/comms/arithmetic(device,dtype,operation,numInputs)
 **********************************************************************/
static Pothos::BlockRegistry registerCommsArithmetic(
    "/arrayfire/comms/arithmetic",
    Pothos::Callable(&makeCommsArithmetic));
