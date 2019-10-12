// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <string>

ArrayFireBlock::ArrayFireBlock(): Pothos::Block()
{
}

ArrayFireBlock::~ArrayFireBlock()
{
}

af::array ArrayFireBlock::getInputPortAsAfArray(size_t portNum)
{
    return Pothos::Object(this->input(portNum)->buffer()).convert<af::array>();
}

af::array ArrayFireBlock::getInputPortAsAfArray(const std::string& portName)
{
    return Pothos::Object(this->input(portName)->buffer()).convert<af::array>();
}

void ArrayFireBlock::postAfArray(size_t portNum, const af::array& afArray)
{
    this->output(portNum)->postBuffer(Pothos::Object(afArray).convert<Pothos::BufferChunk>());
}

void ArrayFireBlock::postAfArray(const std::string& portName, const af::array& afArray)
{
    this->output(portName)->postBuffer(Pothos::Object(afArray).convert<Pothos::BufferChunk>());
}
