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

static Pothos::BlockRegistry registerArithIsInf(
    "/arrayfire/arith/isinf",
    Pothos::Callable(&makeisX)
        .bind<DTypeSupport>({false,false,true,false}, 2)
        .bind<OneToOneFunc>(&af::isInf, 3));

static Pothos::BlockRegistry registerArithIsNaN(
    "/arrayfire/arith/isnan",
    Pothos::Callable(&makeisX)
        .bind<DTypeSupport>({false,false,true,false}, 2)
        .bind<OneToOneFunc>(&af::isNaN, 3));

static Pothos::BlockRegistry registerArithIsZero(
    "/arrayfire/arith/iszero",
    Pothos::Callable(&makeisX)
        .bind<DTypeSupport>({true,true,true,true}, 2)
        .bind<OneToOneFunc>(&af::iszero, 3));
