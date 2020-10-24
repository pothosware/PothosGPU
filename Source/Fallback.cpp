// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "OneToOneBlock.hpp"

#include <arrayfire.h>

// These fallback blocks will be created if the ArrayFire version
// is earlier than when the given underlying function as added.
// As these calls are implemented with ArrayFire, they should
// still be more performant than their PothosBlocks or PothosComms
// equivalents, but they will be slower than the native ArrayFire
// calls.
//
// Note: GenBlocks.py will still generate the documentation, so
// we don't need to do it here.

#if AF_API_VERSION < 37

static af::array afRSqrt(const af::array& afArray)
{
    return 1.0f / af::sqrt(afArray);
}

static Pothos::BlockRegistry registerRSqrt(
    "/gpu/arith/rsqrt",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&afRSqrt, 1)
        .bind<DTypeSupport>({
            true,
            false,
            true,
            false,
        }, 3));

#endif
