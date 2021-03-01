// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#ifdef POTHOSGPU_LEGACY_BUFFER_MANAGER
using BufferAllocateFcn = std::function<Pothos::SharedBuffer(const Pothos::BufferManagerArgs&)>;
#else
using BufferAllocateFcn = Pothos::BufferManager::AllocateFcn;
#endif

Pothos::SharedBuffer allocateSharedBuffer(af::Backend backend, size_t size);

BufferAllocateFcn getSharedBufferAllocator(af::Backend backend);
