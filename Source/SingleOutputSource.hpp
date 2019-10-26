// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

using SingleOutputFunc = af::array(*)(const dim_t, const af::dtype);

class SingleOutputSource: public ArrayFireBlock
{
    public:
        //
        // Factories
        //

        static Pothos::Block* make(
            const SingleOutputFunc& func,
            const Pothos::DType& dtype,
            const DTypeSupport& supportedTypes);

        //
        // Class implementation
        //

        // Since we post all buffers but don't have an input size
        // to match.
        static constexpr dim_t OutputBufferSize = 1024;

        SingleOutputSource(
            const SingleOutputFunc& func,
            const Pothos::DType& dtype);

        virtual ~SingleOutputSource();

        void work() override;

    private:
        SingleOutputFunc _func;
        af::dtype _afDType;
};
