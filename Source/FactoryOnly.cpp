// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "TwoToOneBlock.hpp"

#include "Functions.hpp"

#include <Pothos/Framework.hpp>

// TODO: add tests

static Pothos::BlockRegistry registerSec(
    "/arrayfire/arith/sec",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(sec, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCsc(
    "/arrayfire/arith/csc",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(csc, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCot(
    "/arrayfire/arith/cot",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(cot, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerPolarToComplex(
    "/arrayfire/arith/polar_to_complex",
    Pothos::Callable(&TwoToOneBlock::makeFloatToComplex)
        .bind(polarToComplex, 1)
        .bind(false, 3));
