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
    return 1.0 / af::cos(afArray);
}

inline af::array csc(const af::array& afArray)
{
    return 1.0 / af::sin(afArray);
}

inline af::array cot(const af::array& afArray)
{
    return 1.0 / af::tan(afArray);
}

inline af::array asec(const af::array& afArray)
{
    return af::acos(1.0 / afArray);
}

inline af::array acsc(const af::array& afArray)
{
    return af::asin(1.0 / afArray);
}

inline af::array acot(const af::array& afArray)
{
    return af::atan(1.0 / afArray);
}

inline af::array sech(const af::array& afArray)
{
    return 1.0 / af::cosh(afArray);
}

inline af::array csch(const af::array& afArray)
{
    return 1.0 / af::sinh(afArray);
}

inline af::array coth(const af::array& afArray)
{
    return 1.0 / af::tanh(afArray);
}

inline af::array asech(const af::array& afArray)
{
    return af::acosh(1.0 / afArray);
}

inline af::array acsch(const af::array& afArray)
{
    return af::asinh(1.0 / afArray);
}

inline af::array acoth(const af::array& afArray)
{
    return af::atanh(1.0 / afArray);
}
