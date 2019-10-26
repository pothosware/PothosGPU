// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

using TwoToOneFunc = af::array(*)(const af::array&, const af::array&);

// Assumption: input types match
class TwoToOneBlock: public ArrayFireBlock
{
    public:
        //
        // Factories
        //

        static Pothos::Block* makeFromOneType(
            const TwoToOneFunc& func,
            const Pothos::DType& dtype,
            const DTypeSupport& supportedTypes);

        static Pothos::Block* makeFromTwoTypes(
            const TwoToOneFunc& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            const DTypeSupport& supportedInputTypes,
            const DTypeSupport& supportedOutputTypes);

        //
        // Class implementation
        //

        TwoToOneBlock(
            const TwoToOneFunc& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType);

        virtual ~TwoToOneBlock();

        void work() override;

    private:
        TwoToOneFunc _func;
};
