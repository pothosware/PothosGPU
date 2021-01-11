// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

Pothos::SharedBuffer allocateSharedBuffer(af::Backend backend, size_t size);

Pothos::BufferManager::AllocateFcn getSharedBufferAllocator(af::Backend backend);
