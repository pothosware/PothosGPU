// Copyright (c) 2020-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <vector>

// To avoid collisions
namespace
{

static Pothos::Block* makeIsX(
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
 * |PothosDoc Is Infinite? (GPU)
 *
 * Calls <b>af::isInf</b> on all inputs. Outputs an <b>Int8</b> stream
 * whose values are either <b>1</b> or <b>0</b>, depending on whether
 * or not the associated element is infinite.
 *
 * |category /GPU/Arith
 * |keywords array infinite infinity
 * |factory /gpu/arith/isinf(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerArithIsInf(
    "/gpu/arith/isinf",
    Pothos::Callable(&makeIsX)
        .bind<DTypeSupport>({false,false,true,false}, 2)
        .bind<OneToOneFunc>(&af::isInf, 3));

/*
 * |PothosDoc Is NaN? (GPU)
 *
 * Calls <b>af::isNaN</b> on all inputs. Outputs an <b>Int8</b> stream
 * whose values are either <b>1</b> or <b>0</b>, depending on whether
 * or not the associated element is NaN (not a number).
 *
 * |category /GPU/Arith
 * |keywords array infinite infinity
 * |factory /gpu/arith/isnan(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerArithIsNaN(
    "/gpu/arith/isnan",
    Pothos::Callable(&makeIsX)
        .bind<DTypeSupport>({false,false,true,false}, 2)
        .bind<OneToOneFunc>(&af::isNaN, 3));

/*
 * |PothosDoc Is Zero? (GPU)
 *
 * Calls <b>af::iszero</b> on all inputs. Outputs an <b>Int8</b> stream
 * whose values are either <b>1</b> or <b>0</b>, depending on whether
 * or not the associated element is zero.
 *
 * |category /GPU/Arith
 * |keywords array infinite infinity
 * |factory /gpu/arith/iszero(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerArithIsZero(
    "/gpu/arith/iszero",
    Pothos::Callable(&makeIsX)
        .bind<DTypeSupport>({true,true,true,true}, 2)
        .bind<OneToOneFunc>(&af::iszero, 3));

/*
 * |PothosDoc Is Negative? (GPU)
 *
 * Calls <b>af::sign</b> on all inputs. Outputs an <b>Int8</b> stream
 * whose values are either <b>1</b> or <b>0</b>, depending on whether
 * or not the associated element is negative.
 *
 * |category /GPU/Arith
 * |keywords array minus negative
 * |factory /gpu/arith/sign(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerArithSign(
    "/gpu/arith/sign",
    Pothos::Callable(&makeIsX)
        .bind<DTypeSupport>({true,false,true,false}, 2)
        .bind<OneToOneFunc>(&af::sign, 3));

}
