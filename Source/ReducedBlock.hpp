// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Config.hpp>
#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#include <string>

using ReducedFunc = af::array(*)(const af::array&, const int);

class POTHOS_LOCAL ReducedBlock: public ArrayFireBlock
{
    public:
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
