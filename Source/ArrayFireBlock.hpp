// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#include <string>

class ArrayFireBlock: public Pothos::Block
{
    public:
        ArrayFireBlock();
        virtual ~ArrayFireBlock();

    protected:

        bool getBlockAssumesArrayFireInputs() const;

        void setBlockAssumesArrayFireInputs(bool value);

        af::array getInputPortAsAfArray(
            size_t portNum,
            bool truncateToMinLength = true);

        af::array getInputPortAsAfArray(
            const std::string& portName,
            bool truncateToMinLength = true);

        void postAfArray(
            size_t portNum,
            const af::array& afArray);

        void postAfArray(
            const std::string& portName,
            const af::array& afArray);

        bool _assumeArrayFireInputs;

    private:

        template <typename T>
        af::array _getInputPortAsAfArray(
            const T& portId,
            bool truncateToMinLength);

        template <typename T>
        void _postAfArray(
            const T& portId,
            const af::array& afArray);
};
