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

inline af::array sec(const af::array& afArray)
{
    return af::constant(1.0, afArray.elements(), afArray.type()) / af::cos(afArray);
}

inline af::array csc(const af::array& afArray)
{
    return af::constant(1.0, afArray.elements(), afArray.type()) / af::sin(afArray);
}

inline af::array cot(const af::array& afArray)
{
    return af::constant(1.0, afArray.elements(), afArray.type()) / af::tan(afArray);
}
