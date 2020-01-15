// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Util/TypeInfo.hpp>

#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include <arrayfire.h>

#include <memory>
#include <vector>

//
// Pothos::BufferChunk <-> af::array
//

template <typename AfArrayType>
Pothos::BufferChunk afArrayTypeToBufferChunk(const AfArrayType& afArray);

Pothos::BufferChunk moveAfArrayToBufferChunk(af::array&& rAfArray);
