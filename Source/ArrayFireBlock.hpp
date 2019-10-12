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

        af::array getInputPortAsAfArray(size_t portNum);
        af::array getInputPortAsAfArray(const std::string& portName);

        void postAfArray(size_t portNum, const af::array& afArray);
        void postAfArray(const std::string& portName, const af::array& afArray);
};
