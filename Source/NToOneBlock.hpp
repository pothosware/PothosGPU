// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Config.hpp>
#include <Pothos/Framework.hpp>

#include <arrayfire.h>

using NToOneFunc = af::array(*)(const af::array&, const af::array&);

class POTHOS_LOCAL NToOneBlock: public ArrayFireBlock
{
    public:
        //
        // Factories
        //

        static Pothos::Block* make(
            const std::string& device,
            const NToOneFunc& func,
            const Pothos::DType& dtype,
            size_t numChannels,
            const DTypeSupport& supportedTypes,
            bool shouldPostBuffer);

        static Pothos::Block* makeCallable(
            const std::string& device,
            const Pothos::Callable& func,
            const Pothos::DType& dtype,
            size_t numChannels,
            const DTypeSupport& supportedTypes,
            bool shouldPostBuffer);

        //
        // Class implementation
        //

        NToOneBlock(
            const std::string& device,
            const NToOneFunc& func,
            const Pothos::DType& dtype,
            size_t numChannels,
            bool shouldPostBuffer);

        NToOneBlock(
            const std::string& device,
            const Pothos::Callable& func,
            const Pothos::DType& dtype,
            size_t numChannels,
            bool shouldPostBuffer);

        virtual ~NToOneBlock();

        void work() override;

    private:
        Pothos::Callable _func;
        size_t _nchans;

        bool _postBuffer;
};

#define AF_ARRAY_OP_N_TO_ONE_FUNC(op) \
    [](const af::array& arr1, const af::array& arr2) -> af::array \
    { \
        return (arr1 op arr2); \
    }
