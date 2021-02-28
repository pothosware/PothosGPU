// Copyright (c) 2020-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

namespace PothosGPU
{

Pothos::SharedBuffer allocateSharedBuffer(af::Backend backend, size_t size);

Pothos::BufferManager::AllocateFcn getSharedBufferAllocator(af::Backend backend);

}
