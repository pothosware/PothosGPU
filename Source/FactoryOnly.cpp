// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "TwoToOneBlock.hpp"

#include "Functions.hpp"

#include <Pothos/Framework.hpp>

// TODO: add tests

static Pothos::BlockRegistry registerSec(
    "/arrayfire/trig/sec",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(sec, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCsc(
    "/arrayfire/trig/csc",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(csc, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCot(
    "/arrayfire/trig/cot",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(cot, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerASec(
    "/arrayfire/trig/asec",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(asec, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACsc(
    "/arrayfire/trig/acsc",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acsc, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACot(
    "/arrayfire/trig/acot",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acot, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerSecH(
    "/arrayfire/trig/sech",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(sech, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCscH(
    "/arrayfire/trig/csch",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(csch, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCotH(
    "/arrayfire/trig/coth",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(coth, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerASecH(
    "/arrayfire/trig/asech",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(asech, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACscH(
    "/arrayfire/trig/acsch",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acsch, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACotH(
    "/arrayfire/trig/acoth",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acoth, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerPolarToComplex(
    "/arrayfire/arith/polar_to_complex",
    Pothos::Callable(&TwoToOneBlock::makeFloatToComplex)
        .bind(polarToComplex, 1)
        .bind(false, 3));

static Pothos::BlockRegistry registerHypot(
    "/arrayfire/arith/hypotenuse",
    Pothos::Callable(&TwoToOneBlock::makeFromOneType)
        .bind(hypotenuse, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3)
        .bind(true, 4 /*allowZeroInBuffer1*/));
