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

#define IfOpThenReducedBlock(opStr, func) \
    if(opStr == operation) \
        return new ReducedBlock(device, &func, dtype, dtype, numChans);

#define IfOpThenReducedBlockInt8(opStr, func) \
    if(opStr == operation) \
        return new ReducedBlock(device, &func, dtype, "int8", numChans);

#define IfOpThenNToOneBlock(op, opStr) \
    if(opStr == operation) \
        return new NToOneBlock(device, NToOneLambda(op), dtype, numChans);

static Pothos::Block* makeArrayArithmetic(
    const std::string& device,
    const std::string& operation,
    const Pothos::DType& dtype,
    size_t numChans)
{
    IfOpThenReducedBlock("ADD", af::sum)
    else IfOpThenNToOneBlock(-, "SUBTRACT")
    else IfOpThenReducedBlock("MULTIPLY", af::product)
    else IfOpThenNToOneBlock(/, "DIVIDE")

    throw Pothos::InvalidArgumentException("Invalid operation", operation);
}

static Pothos::Block* makeArrayBitwise(
    const std::string& device,
    const std::string& operation,
    const Pothos::DType& dtype,
    size_t numChans)
{
    if(isDTypeAnyInt(dtype))
    {
        IfOpThenNToOneBlock(&, "AND")
        else IfOpThenNToOneBlock(|, "OR")
        else IfOpThenNToOneBlock(^, "XOR")

        throw Pothos::InvalidArgumentException("Invalid operation", operation);
    }

    throw Pothos::InvalidArgumentException("Invalid type", dtype.name());
}

static Pothos::Block* makeArrayLogical(
    const std::string& device,
    const std::string& operation,
    const Pothos::DType& dtype,
    size_t numChans)
{
    if(isDTypeAnyInt(dtype))
    {
        IfOpThenReducedBlockInt8("AND", af::allTrue)
        else IfOpThenReducedBlockInt8("OR", af::anyTrue)

        throw Pothos::InvalidArgumentException("Invalid operation", operation);
    }

    throw Pothos::InvalidArgumentException("Invalid type", dtype.name());
}

// TODO: TwoToOne: comparators, << >>

//
// Block registries
//

/*
 * |PothosDoc Arithmetic
 *
 * Perform the specified arithmetic operation on all given inputs, resulting
 * in a single output stream.
 *
 * |category /ArrayFire/Array
 * |keywords array add subtract multiply divide modulus
 * |factory /arrayfire/array/arithmetic(device,operation,dtype,nchans)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param operation[Operation] The arithmetic operation to perform.
 * |widget ComboBox(editable=false)
 * |option [Add] "ADD"
 * |option [Subtract] "SUBTRACT"
 * |option [Multiply] "MULTIPLY"
 * |option [Divide] "DIVIDE"
 * |default "ADD"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1)
 * |default "float64"
 * |preview disable
 *
 * |param nchans[Num Channels] The number of arithmetic inputs.
 * |widget SpinBox(minimum=2)
 * |default 2
 * |preview disable
 */
static Pothos::BlockRegistry registerArrayArithmetic(
    "/arrayfire/array/arithmetic",
    Pothos::Callable(&makeArrayArithmetic));

/*
 * |PothosDoc Bitwise
 *
 * Perform the specified bitwise operation on all given inputs, resulting
 * in a single output stream.
 *
 * |category /ArrayFire/Array
 * |keywords array and or xor
 * |factory /arrayfire/array/bitwise(device,operation,dtype,nchans)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param operation[Operation] The bitwise operation to perform.
 * |widget ComboBox(editable=false)
 * |option [And] "AND"
 * |option [Or] "OR"
 * |option [XOR] "XOR"
 * |default "AND"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1)
 * |default "uint64"
 * |preview disable
 *
 * |param nchans[Num Channels] The number of inputs.
 * |widget SpinBox(minimum=2)
 * |default 2
 * |preview disable
 */
static Pothos::BlockRegistry registerArrayBitwise(
    "/arrayfire/array/bitwise",
    Pothos::Callable(&makeArrayBitwise));

/*
 * |PothosDoc Logical
 *
 * Perform the specified logical operation on all given inputs, resulting
 * in a single output stream of type <b>int8</b>, where <b>0</b> corresponds
 * to false and <b>1</b> corresponds to true.
 *
 * |category /ArrayFire/Array
 * |keywords array and or xor
 * |factory /arrayfire/array/logical(device,operation,dtype,nchans)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param operation[Operation] The logical operation to perform.
 * |widget ComboBox(editable=false)
 * |option [And] "AND"
 * |option [Or] "OR"
 * |default "AND"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1)
 * |default "uint64"
 * |preview disable
 *
 * |param nchans[Num Channels] The number of inputs.
 * |widget SpinBox(minimum=2)
 * |default 2
 * |preview disable
 */
static Pothos::BlockRegistry registerArrayLogical(
    "/arrayfire/array/logical",
    Pothos::Callable(&makeArrayLogical));
