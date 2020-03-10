// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "TwoToOneBlock.hpp"
#include "NToOneBlock.hpp"

#include "Functions.hpp"

#include <Pothos/Framework.hpp>

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

static Pothos::BlockRegistry registerASec(
    "/arrayfire/arith/asec",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(asec, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACsc(
    "/arrayfire/arith/acsc",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acsc, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACot(
    "/arrayfire/arith/acot",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acot, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerSecH(
    "/arrayfire/arith/sech",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(sech, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCscH(
    "/arrayfire/arith/csch",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(csch, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCotH(
    "/arrayfire/arith/coth",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(coth, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerASecH(
    "/arrayfire/arith/asech",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(asech, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACscH(
    "/arrayfire/arith/acsch",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acsch, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACotH(
    "/arrayfire/arith/acoth",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acoth, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerPolarToComplex(
    "/arrayfire/arith/polar_to_complex",
    Pothos::Callable(&TwoToOneBlock::makeFloatToComplex)
        .bind(polarToComplex, 1)
        .bind(false, 3));

static Pothos::BlockRegistry registerSinc(
    "/arrayfire/signal/sinc",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(sinc, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerSetUnique(
    "/arrayfire/algorithm/set_unique",
    Pothos::Callable(&OneToOneBlock::makeFromOneTypeCallable)
        .bind(Pothos::Callable(af::setUnique).bind(false, 1), 1)
        .bind<DTypeSupport>({true,true,true,false}, 3));

static Pothos::BlockRegistry registerSetUnion(
    "/arrayfire/algorithm/set_union",
    Pothos::Callable(&NToOneBlock::makeCallable)
        .bind(Pothos::Callable(af::setUnion).bind(false, 2), 1)
        .bind<DTypeSupport>({true,true,true,false}, 4));

static Pothos::BlockRegistry registerFlip(
    "/arrayfire/data/flip",
    Pothos::Callable(&OneToOneBlock::makeFromOneTypeCallable)
        .bind(Pothos::Callable(af::flip).bind(0, 1), 1)
        .bind<DTypeSupport>({true,true,true,true}, 3));
