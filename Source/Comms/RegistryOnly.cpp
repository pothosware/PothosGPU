// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-Clause-3

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Plugin.hpp>

#include <arrayfire.h>

static Pothos::BlockRegistry registerCommsAbs(
    "/arrayfire/comms/abs",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::abs, 1)
        .bind(DTypeSupport{true,true,true,true}, 3));

static Pothos::BlockRegistry registerCommsAngle(
    "/arrayfire/comms/angle",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::arg, 1)
        .bind(DTypeSupport{false,false,false,true}, 3));

static Pothos::BlockRegistry registerCommsCombineComplex(
    "/arrayfire/comms/combine_complex",
    Pothos::Callable(&OneToOneBlock::makeFloatToComplex)
        .bind<OneToOneFunc>(&af::complex, 1));

static Pothos::BlockRegistry registerCommsConjugate(
    "/arrayfire/comms/conjugate",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::conjg, 1)
        .bind(DTypeSupport{false,false,false,true}, 3));

static Pothos::BlockRegistry registerCommsLog10(
    "/arrayfire/comms/log10",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(&af::log10, 1)
        .bind(DTypeSupport{false,false,true,false}, 3));
