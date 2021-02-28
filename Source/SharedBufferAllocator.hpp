// Copyright (c) 2020-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Config.hpp>
#include <Pothos/Framework.hpp>

#include <arrayfire.h>

POTHOS_LOCAL Pothos::SharedBuffer allocateSharedBuffer(af::Backend backend, size_t size);

POTHOS_LOCAL Pothos::BufferManager::AllocateFcn getSharedBufferAllocator(af::Backend backend);
