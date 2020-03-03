// Copyright (c) 2013-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BufferConversions.hpp"
#include "DeviceCache.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Util/OrderedQueue.hpp>
#include <Pothos/Framework/BufferManager.hpp>

#include <arrayfire.h>

#include <cassert>
#include <iostream>

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
        af::setBackend(_backend);
        af::setDevice(0);

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
 * Factory
 **********************************************************************/
Pothos::BufferManager::Sptr makePinnedBufferManager(
    af::Backend backend,
    const Pothos::BufferManagerArgs& args)
{
    auto bufferManager = std::make_shared<PinnedBufferManager>(backend);
    bufferManager->init(args);
    return bufferManager;
}
