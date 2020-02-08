// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#include <string>

using OneToOneFunc = af::array(*)(const af::array&);

class OneToOneBlock: public ArrayFireBlock
{
    public:
        //
        // Factories
        //

        static Pothos::Block* makeFromOneType(
            const std::string& device,
            const OneToOneFunc& func,
            const Pothos::DType& dtype,
            const DTypeSupport& supportedTypes);

        static Pothos::Block* makeFloatToComplex(
            const std::string& device,
            const OneToOneFunc& func,
            const Pothos::DType& floatType);

        static Pothos::Block* makeComplexToFloat(
            const std::string& device,
            const OneToOneFunc& func,
            const Pothos::DType& floatType);

        //
        // Class implementation
        //

        OneToOneBlock(
            const std::string& device,
            const OneToOneFunc& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType);

        OneToOneBlock(
            const std::string& device,
            const Pothos::Callable& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType);

        virtual ~OneToOneBlock();

        void work() override;

    protected:

        Pothos::Callable _func;

        // We need to store this since ArrayFire may change the output type.
        af::dtype _afOutputDType;
};
