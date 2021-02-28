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
            const std::string& device,
            const TwoToOneFunc& func,
            const Pothos::DType& dtype,
            const DTypeSupport& supportedTypes,
            bool allowZeroInBuffer1);

        static Pothos::Block* makeFloatToComplex(
            const std::string& device,
            const TwoToOneFunc& func,
            const Pothos::DType& floatType,
            bool allowZeroInBuffer1);

        static Pothos::Block* makeComparator(
            const std::string& device,
            const TwoToOneFunc& func,
            const Pothos::DType& dtype,
            const DTypeSupport& supportedTypes);

        //
        // Class implementation
        //

        TwoToOneBlock(
            const std::string& device,
            const TwoToOneFunc& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            bool allowZeroInBuffer1);

        virtual ~TwoToOneBlock();

        void work() override;

    private:
        TwoToOneFunc _func;
        bool _allowZeroInBuffer1;
};

#define AF_ARRAY_OP_TWO_TO_ONE_FUNC(op) \
    [](const af::array& arr1, const af::array& arr2) -> af::array \
    { \
        return (arr1 op arr2); \
    }
