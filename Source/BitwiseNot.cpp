// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#if AF_API_VERSION >= 38

static af::array afNot(const af::array& afArray)
{
    return ~afArray;
}

#else



#endif

/*
 * |PothosDoc Bitwise Not
 *
 * Perform the bitwise not operation on all inputs, returning
 * the outputs in the output stream.
 *
 * |category /GPU/Array
 * |factory /gpu/array/bitwise_not(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,dim=1)
 * |default "uint64"
 * |preview disable
 */
static Pothos::BlockRegistry registerBitwiseNot(
    "/gpu/array/bitwise_not",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&afNot, 1)
        .bind<DTypeSupport>({
            true,
            true,
            false,
            false,
        }, 3)
);
