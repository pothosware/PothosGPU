// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "TwoToOneBlock.hpp"
#include "NToOneBlock.hpp"

#include "Functions.hpp"

#include <Pothos/Framework.hpp>

static Pothos::BlockRegistry registerSec(
    "/gpu/arith/sec",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(sec, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCsc(
    "/gpu/arith/csc",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(csc, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCot(
    "/gpu/arith/cot",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(cot, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerASec(
    "/gpu/arith/asec",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(asec, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACsc(
    "/gpu/arith/acsc",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acsc, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACot(
    "/gpu/arith/acot",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acot, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerSecH(
    "/gpu/arith/sech",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(sech, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCscH(
    "/gpu/arith/csch",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(csch, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerCotH(
    "/gpu/arith/coth",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(coth, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerASecH(
    "/gpu/arith/asech",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(asech, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACscH(
    "/gpu/arith/acsch",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acsch, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerACotH(
    "/gpu/arith/acoth",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(acoth, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerSinc(
    "/gpu/signal/sinc",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(sinc, 1)
        .bind<DTypeSupport>({false,false,true,false}, 3));

static Pothos::BlockRegistry registerSetUnique(
    "/gpu/algorithm/set_unique",
    Pothos::Callable(&OneToOneBlock::makeFromOneTypeCallable)
        .bind(Pothos::Callable(af::setUnique).bind(false, 1), 1)
        .bind<DTypeSupport>({true,true,true,false}, 3));

static Pothos::BlockRegistry registerSetUnion(
    "/gpu/algorithm/set_union",
    Pothos::Callable(&NToOneBlock::makeCallable)
        .bind(Pothos::Callable(af::setUnion).bind(false, 2), 1)
        .bind<DTypeSupport>({true,true,true,false}, 4));

static Pothos::BlockRegistry registerFlip(
    "/gpu/data/flip",
    Pothos::Callable(&OneToOneBlock::makeFromOneTypeCallable)
        .bind(Pothos::Callable(af::flip).bind(0, 1), 1)
        .bind<DTypeSupport>({true,true,true,true}, 3));
