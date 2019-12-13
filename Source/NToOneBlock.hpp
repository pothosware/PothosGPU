// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

using NToOneFunc = af::array(*)(const af::array&, const af::array&);

class NToOneBlock: public ArrayFireBlock
{
    public:
        //
        // Factories
        //

        static Pothos::Block* make(
            const NToOneFunc& func,
            const Pothos::DType& dtype,
            size_t numChannels,
            const DTypeSupport& supportedTypes);

        //
        // Class implementation
        //

        NToOneBlock(
            const NToOneFunc& func,
            const Pothos::DType& dtype,
            size_t numChannels);

        virtual ~NToOneBlock();

        void work() override;

    private:
        NToOneFunc _func;
        size_t _nchans;
};

#define AF_ARRAY_OP_N_TO_ONE_FUNC(op) \
    [](const af::array& arr1, const af::array& arr2) -> af::array \
    { \
        return (arr1 op arr2); \
    }
