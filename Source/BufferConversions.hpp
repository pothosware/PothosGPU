// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#include <memory>

//
// Minimal wrapper class to ensure allocation and deallocation are done
// with the same backend.
//

class AfPinnedMemRAII
{
    public:
        using SPtr = std::shared_ptr<AfPinnedMemRAII>;
        
        AfPinnedMemRAII(af::Backend backend, size_t allocSize);

        ~AfPinnedMemRAII();

        inline void* get() const
        {
            return _pinnedMem;
        }

    private:
        af::Backend _backend;
        void* _pinnedMem;
};

Pothos::BufferManager::Sptr makePinnedBufferManager(af::Backend backend);

//
// Pothos::BufferChunk <-> af::array
//

template <typename AfArrayType>
Pothos::BufferChunk afArrayTypeToBufferChunk(const AfArrayType& afArray);
