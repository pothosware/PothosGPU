// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

//
// Pothos::BufferChunk <-> af::array
//

template <typename AfArrayType>
Pothos::BufferChunk afArrayTypeToBufferChunk(const AfArrayType& afArray);
