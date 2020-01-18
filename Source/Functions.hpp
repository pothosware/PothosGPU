// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <arrayfire.h>

// Equivalent of std::polar
af::array polarToComplex(const af::array& rho, const af::array& theta)
{
    return af::complex((rho * af::cos(theta)), (rho * af::sin(theta)));
};
