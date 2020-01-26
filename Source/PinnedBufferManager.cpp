// Copyright (c) 2013-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Util/OrderedQueue.hpp>
#include <Pothos/Framework/BufferManager.hpp>

#include <arrayfire.h>

#include <cassert>
#include <iostream>

// Minimal wrapper class to ensure allocation and deallocation are done
// with the same backend.
class AfPinnedMemRAII
{
    public:
        AfPinnedMemRAII(af::Backend backend, size_t allocSize):
            _backend(backend),
            _pinnedMem(nullptr)
        {
            af::setBackend(_backend);
            _pinnedMem = af::pinned(allocSize, ::u8);
        }

        ~AfPinnedMemRAII()
        {
            try
            {
                af::setBackend(_backend);
                af::freePinned(_pinnedMem);
            }
            catch(...){}
        }

        void* get() const
        {
            return _pinnedMem;
        }

    private:
        af::Backend _backend;
        void* _pinnedMem;
};

/************************************************************************
 * Identical to Pothos::GenericBufferManager but uses page-locked memory
 * allocated by ArrayFire.
 ************************************************************************/
class PinnedBufferManager :
    public Pothos::BufferManager,
    public std::enable_shared_from_this<PinnedBufferManager>
{
public:
    PinnedBufferManager(af::Backend backend):
        _backend(backend),
        _bufferSize(0),
        _bytesPopped(0)
    {
        return;
    }

    void init(const Pothos::BufferManagerArgs &args)
    {
        Pothos::BufferManager::init(args);
        _bufferSize = args.bufferSize;
        _readyBuffs = Pothos::Util::OrderedQueue<Pothos::ManagedBuffer>(args.numBuffers);

        //allocate one large continuous slab
        const size_t totalSize = args.bufferSize*args.numBuffers;
        
        auto afPinnedMemSPtr = std::make_shared<AfPinnedMemRAII>(
                                  _backend,
                                  totalSize);
        auto commonSlab = Pothos::SharedBuffer(
                              reinterpret_cast<size_t>(afPinnedMemSPtr->get()),
                              totalSize,
                              afPinnedMemSPtr);

        //create managed buffers based on chunks from the slab
        std::vector<Pothos::ManagedBuffer> managedBuffers(args.numBuffers);
        for (size_t i = 0; i < args.numBuffers; i++)
        {
            const size_t addr = commonSlab.getAddress()+(args.bufferSize*i);
            Pothos::SharedBuffer sharedBuff(addr, args.bufferSize, commonSlab);
            managedBuffers[i].reset(this->shared_from_this(), sharedBuff, i/*slabIndex*/);
            this->push(managedBuffers[i]);
        }

        //set the next buffer pointers
        for (size_t i = 0; i < managedBuffers.size()-1; i++)
        {
            managedBuffers[i].setNextBuffer(managedBuffers[i+1]);
        }
    }

    bool empty(void) const
    {
        return _readyBuffs.empty();
    }

    void pop(const size_t numBytes)
    {
        assert(not _readyBuffs.empty());
        _bytesPopped += numBytes;

        //re-use the buffer for small consumes
        if (_bytesPopped*2 < _bufferSize)
        {
            auto buff = this->front();
            buff.address += numBytes;
            buff.length -= numBytes;
            this->setFrontBuffer(buff);
            return;
        }

        _bytesPopped = 0;
        _readyBuffs.pop();
        if (_readyBuffs.empty()) this->setFrontBuffer(Pothos::BufferChunk::null());
        else this->setFrontBuffer(_readyBuffs.front());
    }

    void push(const Pothos::ManagedBuffer &buff)
    {
        if (_readyBuffs.empty()) this->setFrontBuffer(buff);
        _readyBuffs.push(buff, buff.getSlabIndex());
    }

private:

    af::Backend _backend;
    size_t _bufferSize;
    size_t _bytesPopped;
    Pothos::Util::OrderedQueue<Pothos::ManagedBuffer> _readyBuffs;
};

/***********************************************************************
 * factory and registration
 **********************************************************************/
static Pothos::BufferManager::Sptr makePinnedBufferManager(void)
{
    af::Backend backend;
    
    const auto& availableBackends = getAvailableBackends();
    if(doesVectorContainValue(availableBackends, ::AF_BACKEND_CUDA))
    {
        backend = ::AF_BACKEND_CUDA;
    }
    else if(doesVectorContainValue(availableBackends, ::AF_BACKEND_OPENCL))
    {
        backend = ::AF_BACKEND_OPENCL;
    }
    else
    {
        throw Pothos::RuntimeException(
                "ArrayFire was not built with CUDA or OpenCL support "
                "and cannot allocate pinned memory.");
    }
    
    return std::make_shared<PinnedBufferManager>(backend);
}

pothos_static_block(pothosFrameworkRegisterPinnedBufferManager)
{
    Pothos::PluginRegistry::addCall(
        "/framework/buffer_manager/pinned",
        &makePinnedBufferManager);
}
