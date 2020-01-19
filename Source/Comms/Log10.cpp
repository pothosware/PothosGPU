// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-Clause-3

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Plugin.hpp>

static Pothos::BlockRegistry registerCommsLog10(
    "/arrayfire/comms/log10",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind(&af::log10, 1)
        .bind(DTypeSupport{false,false,true,false}, 3));
