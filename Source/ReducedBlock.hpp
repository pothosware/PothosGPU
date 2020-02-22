// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#include <string>

using ReducedFunc = af::array(*)(const af::array&, const int);

class ReducedBlock: public ArrayFireBlock
{
    public:
        //
        // Factories
        //

        static Pothos::Block* makeFromOneType(
            const std::string& device,
            const ReducedFunc& func,
            const Pothos::DType& dtype,
            size_t numChannels,
            const DTypeSupport& supportedTypes);

        static Pothos::Block* makeInt8Out(
            const std::string& device,
            const ReducedFunc& func,
            const Pothos::DType& dtype,
            size_t numChannels,
            const DTypeSupport& supportedTypes);

        //
        // Class implementation
        //

        ReducedBlock(
            const std::string& device,
            const ReducedFunc& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            size_t numChannels);

        virtual ~ReducedBlock();

        af::array getNumberedInputPortsAs2DAfArray();

        void work() override;

    private:
        ReducedFunc _func;
        af::dtype _afOutputDType;
        size_t _nchans;
};
