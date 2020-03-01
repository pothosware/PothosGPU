// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <vector>

static Pothos::Block* makeisX(
    const std::string& device,
    const Pothos::DType& dtype,
    const DTypeSupport& dtypeSupport,
    OneToOneFunc func)
{
    static const Pothos::DType Int8DType("int8");

    validateDType(dtype, dtypeSupport);

    return new OneToOneBlock(
                   device,
                   func,
                   dtype,
                   Int8DType);
}

/*
 * |PothosDoc Is Infinite?
 *
 * Calls <b>af::isinf</b> on all inputs. Outputs an <b>Int8</b> stream
 * whose values are either <b>1</b> or <b>0</b>, depending on whether
 * or not the associated element is infinite.
 *
 * |category /ArrayFire/Arith
 * |keywords array infinite infinity
 * |factory /arrayfire/arith/isinf(device,dtype)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param dtype(Data Type) The output's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerArithIsInf(
    "/arrayfire/arith/isinf",
    Pothos::Callable(&makeisX)
        .bind<DTypeSupport>({false,false,true,false}, 2)
        .bind<OneToOneFunc>(&af::isInf, 3));

/*
 * |PothosDoc Is NaN?
 *
 * Calls <b>af::isnan</b> on all inputs. Outputs an <b>Int8</b> stream
 * whose values are either <b>1</b> or <b>0</b>, depending on whether
 * or not the associated element is NaN (not a number).
 *
 * |category /ArrayFire/Arith
 * |keywords array infinite infinity
 * |factory /arrayfire/arith/isnan(device,dtype)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param dtype(Data Type) The output's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerArithIsNaN(
    "/arrayfire/arith/isnan",
    Pothos::Callable(&makeisX)
        .bind<DTypeSupport>({false,false,true,false}, 2)
        .bind<OneToOneFunc>(&af::isNaN, 3));

/*
 * |PothosDoc Is Zero?
 *
 * Calls <b>af::isnan</b> on all inputs. Outputs an <b>Int8</b> stream
 * whose values are either <b>1</b> or <b>0</b>, depending on whether
 * or not the associated element is zero.
 *
 * |category /ArrayFire/Arith
 * |keywords array infinite infinity
 * |factory /arrayfire/arith/iszero(device,dtype)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param dtype(Data Type) The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerArithIsZero(
    "/arrayfire/arith/iszero",
    Pothos::Callable(&makeisX)
        .bind<DTypeSupport>({true,true,true,true}, 2)
        .bind<OneToOneFunc>(&af::iszero, 3));
