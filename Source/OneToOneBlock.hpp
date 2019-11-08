// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>

#include <arrayfire.h>

using OneToOneFunc = af::array(*)(const af::array&);

class OneToOneBlock: public ArrayFireBlock
{
    public:
        //
        // Factories
        //

        static Pothos::Block* makeFromOneType(
            const OneToOneFunc& func,
            const Pothos::DType& dtype,
            const DTypeSupport& supportedTypes,
            size_t numChans);

        static Pothos::Block* makeFromTwoTypes(
            const OneToOneFunc& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            const DTypeSupport& supportedInputTypes,
            const DTypeSupport& supportedOutputTypes,
            size_t numChans);

        //
        // Class implementation
        //

        OneToOneBlock(
            const OneToOneFunc& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            size_t numChans);

        OneToOneBlock(
            const Pothos::Callable& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            size_t numChans);

        virtual ~OneToOneBlock();

        void work() override;

    protected:

        af::array getInputsAsAfArray();

        void work(const af::array& afInput);

        void post2DAfArrayToNumberedOutputPorts(const af::array& afArray);

    private:
        Pothos::Callable _func;
        size_t _nchans;

        // We need to store this since ArrayFire may change the output value.
        af::dtype _afOutputDType;
};
