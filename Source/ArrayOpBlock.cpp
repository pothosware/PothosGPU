// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "NToOneBlock.hpp"
#include "ReducedBlock.hpp"
#include "TwoToOneBlock.hpp"
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

#define IfOpThenTwoToOneBlock(op, opStr) \
    if(opStr == operation) \
        return new TwoToOneBlock(device, NToOneLambda(op), dtype, dtype, true);

static Pothos::Block* makeArrayArithmetic(
    const std::string& device,
    const std::string& operation,
    const Pothos::DType& dtype,
    size_t numChans)
{
    static const DTypeSupport allDTypeSupport{true,true,true,true};
    static const DTypeSupport modulusDTypeSupport{true,true,true,false};

    const auto& dtypeSupport = ("Modulus" == operation) ? modulusDTypeSupport : allDTypeSupport;
    validateDType(dtype, dtypeSupport);

    IfOpThenReducedBlock("Add", af::sum)
    else IfOpThenNToOneBlock(-, "Subtract")
    else IfOpThenReducedBlock("Multiply", af::product)
    else IfOpThenNToOneBlock(/, "Divide")
    else IfOpThenNToOneBlock(%, "Modulus")

    throw Pothos::InvalidArgumentException("Invalid operation", operation);
}

static Pothos::Block* makeComparator(
    const std::string& device,
    const std::string& operation,
    const Pothos::DType& dtype)
{
    static const DTypeSupport dtypeSupport{true,true,true,false};

    #define IfOpThenComparator(op) \
        if(#op == operation) \
            return TwoToOneBlock::makeComparator(device, NToOneLambda(op), dtype, dtypeSupport);

    IfOpThenComparator(<)
    else IfOpThenComparator(<=)
    else IfOpThenComparator(>)
    else IfOpThenComparator(>=)
    else IfOpThenComparator(==)
    else IfOpThenComparator(!=)

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
        IfOpThenNToOneBlock(&, "And")
        else IfOpThenNToOneBlock(|, "Or")
        else IfOpThenNToOneBlock(^, "XOr")
        else IfOpThenTwoToOneBlock(<<, "Left Shift")
        else IfOpThenTwoToOneBlock(>>, "Right Shift")

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
        IfOpThenReducedBlockInt8("And", af::allTrue)
        else IfOpThenReducedBlockInt8("Or", af::anyTrue)

        throw Pothos::InvalidArgumentException("Invalid operation", operation);
    }

    throw Pothos::InvalidArgumentException("Invalid type", dtype.name());
}

//
// Block registries
//

/*
 * |PothosDoc Arithmetic
 *
 * Perform the specified arithmetic operation on all given inputs, resulting
 * in a single output stream.
 *
 * |category /GPU/Array
 * |keywords array add subtract multiply divide modulus
 * |factory /gpu/array/arithmetic(device,operation,dtype,nchans)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param operation[Operation] The arithmetic operation to perform.
 * |widget ComboBox(editable=false)
 * |option [Add] "Add"
 * |option [Subtract] "Subtract"
 * |option [Multiply] "Multiply"
 * |option [Divide] "Divide"
 * |option [Modulus] "Modulus"
 * |default "Add"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param nchans[Num Channels] The number of arithmetic inputs.
 * |widget SpinBox(minimum=2)
 * |default 2
 * |preview disable
 */
static Pothos::BlockRegistry registerArrayArithmetic(
    "/gpu/array/arithmetic",
    Pothos::Callable(&makeArrayArithmetic));

/*
 * |PothosDoc Comparator
 *
 * Perform the specified comparison, using port <b>0</b> for the first
 * operand and port <b>1</b> for the second operand, resulting in
 * a single output stream of type <b>int8</b>, where <b>0</b>
 * corresponds to false and <b>1</b> corresponds to true.
 *
 * |category /GPU/Array
 * |keywords array less greater equal
 * |factory /gpu/array/comparator(device,operation,dtype,dim=1)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param operation[Operation] The arithmetic operation to perform.
 * |widget ComboBox(editable=false)
 * |option [<] "<"
 * |option [<=] "<="
 * |option [>] ">"
 * |option [>=] ">="
 * |option [==] "=="
 * |option [!=] "!="
 * |default "<"
 * |preview enable
 *
 * |param dtype[Data Type] The input's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerComparator(
    "/gpu/array/comparator",
    Pothos::Callable(&makeComparator));

/*
 * |PothosDoc Bitwise
 *
 * Perform the specified bitwise operation on all given inputs, resulting
 * in a single output stream.
 *
 * <b>Note:</b> the <b>Left Shift</b> and <b>Right Shift</b> operations
 * automatically use two inputs.
 *
 * |category /GPU/Array
 * |keywords array and or xor left right shift
 * |factory /gpu/array/bitwise(device,operation,dtype,nchans)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param operation[Operation] The bitwise operation to perform.
 * |widget ComboBox(editable=false)
 * |option [And] "And"
 * |option [Or] "Or"
 * |option [XOr] "XOr"
 * |option [Left Shift] "Left Shift"
 * |option [Right Shift] "Right Shift"
 * |default "And"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,dim=1)
 * |default "uint64"
 * |preview disable
 *
 * |param nchans[Num Channels] The number of inputs.
 * |widget SpinBox(minimum=2)
 * |default 2
 * |preview disable
 */
static Pothos::BlockRegistry registerArrayBitwise(
    "/gpu/array/bitwise",
    Pothos::Callable(&makeArrayBitwise));

/*
 * |PothosDoc Logical
 *
 * Perform the specified logical operation on all given inputs, resulting
 * in a single output stream of type <b>int8</b>, where <b>0</b> corresponds
 * to false and <b>1</b> corresponds to true.
 *
 * |category /GPU/Array
 * |keywords array and or xor
 * |factory /gpu/array/logical(device,operation,dtype,nchans)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param operation[Operation] The logical operation to perform.
 * |widget ComboBox(editable=false)
 * |option [And] "And"
 * |option [Or] "Or"
 * |default "And"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,dim=1)
 * |default "uint64"
 * |preview disable
 *
 * |param nchans[Num Channels] The number of inputs.
 * |widget SpinBox(minimum=2)
 * |default 2
 * |preview disable
 */
static Pothos::BlockRegistry registerArrayLogical(
    "/gpu/array/logical",
    Pothos::Callable(&makeArrayLogical));
