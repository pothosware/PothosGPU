// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TwoToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#define TwoToOneLambda(op) \
    [](const af::array& a, const af::array& b){return a op b;}

#define IfOpThenTwoToOneBlock(op) \
    if(#op == comparator) \
        return TwoToOneBlock::makeComparator(device, TwoToOneLambda(op), dtype, dtypeSupport);

static Pothos::Block* makeCommsComparator(
    const std::string& device,
    const Pothos::DType& dtype,
    const std::string& comparator)
{
    static const DTypeSupport dtypeSupport{true,true,true,false};

    IfOpThenTwoToOneBlock(>)
    else IfOpThenTwoToOneBlock(<)
    else IfOpThenTwoToOneBlock(>=)
    else IfOpThenTwoToOneBlock(<=)
    else IfOpThenTwoToOneBlock(==)
    else IfOpThenTwoToOneBlock(!=)

    throw Pothos::InvalidArgumentException("Invalid operation", comparator);
}

/***********************************************************************
 * |PothosDoc Comparator (Parallel)
 *
 * Perform a comparision between 2 inputs and outputs a char value of 1 or 0
 *
 * out[n] = (in0[n] $op in1[n]) ? 1 : 0;
 *
 * |category /ArrayFire/Comms
 * |keywords math logic comparator
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 * |widget ComboBox(editable=false)
 * |preview enable
 *
 * |param dtype[Data Type] The data type used in the arithmetic.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param comparator The comparison operation to perform
 * |default ">"
 * |option [>] ">"
 * |option [<] "<"
 * |option [>=] ">="
 * |option [<=] "<="
 * |option [==] "=="
 * |option [!=] "!="
 *
 * |factory /arrayfire/comms/comparator(device,dtype,comparator)
 **********************************************************************/
static Pothos::BlockRegistry registerCommsComparator(
    "/arrayfire/comms/comparator",
    Pothos::Callable(&makeCommsComparator));
