// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <vector>

static Pothos::Block* makeIsInfNaN(
    const std::string& device,
    const Pothos::DType& dtype,
    OneToOneFunc func,
    size_t nchans)
{
    static const Pothos::DType Int8DType("int8");
    static const DTypeSupport DTypeSupport{false,false,true,false};

    validateDType(dtype, DTypeSupport);

    return new OneToOneBlock(
                   device,
                   func,
                   dtype,
                   Int8DType,
                   nchans);
}

static Pothos::BlockRegistry registerArithIsInf(
    "/arrayfire/arith/isinf",
    Pothos::Callable(&makeIsInfNaN)
        .bind<OneToOneFunc>(&af::isInf, 1));

static Pothos::BlockRegistry registerArithIsNaN(
    "/arrayfire/arith/isnan",
    Pothos::Callable(&makeIsInfNaN)
        .bind<OneToOneFunc>(&af::isNaN, 1));
