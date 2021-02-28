// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "SharedBufferAllocator.hpp"

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#include <memory>

// To avoid collisions
namespace
{

//
// Minimal wrapper class to ensure allocation and deallocation are done
// with the same backend.
//

class AfPinnedMemRAII
{
    public:
        using SPtr = std::shared_ptr<AfPinnedMemRAII>;
        
        AfPinnedMemRAII(af::Backend backend, size_t allocSize):
            _backend(backend),
            _pinnedMem(nullptr)
        {
            af::setBackend(_backend);
            _pinnedMem = af::pinned(allocSize, ::u8);
        }

        virtual ~AfPinnedMemRAII()
        {
            try
            {
                af::setBackend(_backend);
                af::freePinned(_pinnedMem);
            }
            catch(...){}
        }

        inline void* get() const
        {
            return _pinnedMem;
        }

    private:
        af::Backend _backend;
        void* _pinnedMem;
};

}

//
// Transparent RAII SharedBuffer
//

Pothos::SharedBuffer allocateSharedBuffer(af::Backend backend, size_t size)
{
    auto afPinnedMemSPtr = std::make_shared<AfPinnedMemRAII>(
                              backend,
                              size);
    return Pothos::SharedBuffer(
              reinterpret_cast<size_t>(afPinnedMemSPtr->get()),
              size,
              afPinnedMemSPtr);
}

Pothos::BufferManager::AllocateFcn getSharedBufferAllocator(af::Backend backend)
{
    auto impl = [backend](const Pothos::BufferManagerArgs& args)
    {
        const size_t totalSize = args.bufferSize*args.numBuffers;
        return allocateSharedBuffer(backend, totalSize);
    };

    return impl;
}
