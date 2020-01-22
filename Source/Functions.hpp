// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <arrayfire.h>

// Equivalent of std::polar
inline af::array polarToComplex(const af::array& rho, const af::array& theta)
{
    return af::complex((rho * af::cos(theta)), (rho * af::sin(theta)));
};

inline af::array logN(const af::array& afArray, double base)
{
    return af::log(afArray) / af::log(af::constant(base, afArray.elements()));
}
