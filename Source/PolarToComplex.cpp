// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TwoToOneBlock.hpp"
#include "Functions.hpp"

#include <Pothos/Framework.hpp>

// TODO: add test
static Pothos::BlockRegistry registerPolarToComplex(
    "/arrayfire/arith/polar_to_complex",
    Pothos::Callable(&TwoToOneBlock::makeFloatToComplex)
        .bind(polarToComplex, 1)
        .bind(false, 3));
