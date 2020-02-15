// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"
#include "Utility.hpp"

#include <Pothos/Managed.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Plugin.hpp>

#include <Poco/Logger.h>

#include <algorithm>

static std::vector<af::Backend> _getAvailableBackends()
{
    const std::vector<af::Backend> AllBackends =
    {
        ::AF_BACKEND_CUDA,
        ::AF_BACKEND_OPENCL,
        ::AF_BACKEND_CPU,
    };
    const int afAvailableBackends = af::getAvailableBackends();

    std::vector<af::Backend> availableBackends;
    std::copy_if(
        AllBackends.begin(),
        AllBackends.end(),
        std::back_inserter(availableBackends),
        [&afAvailableBackends](af::Backend backend)
        {
            return (afAvailableBackends & backend);
        });

    return availableBackends;
}

static std::vector<DeviceCacheEntry> _getDeviceCache()
{
    std::vector<DeviceCacheEntry> deviceCache;

    for(const auto& backend: getAvailableBackends())
    {
        af::setBackend(backend);

        // For current backend
        const int numDevices = af::getDeviceCount();
        for(int devIndex = 0; devIndex < numDevices; ++devIndex)
        {
            static constexpr size_t bufferLen = 1024;

            char name[bufferLen] = {0};
            char platform[bufferLen] = {0};
            char toolkit[bufferLen] = {0};
            char compute[bufferLen] = {0};
            af::setDevice(devIndex);
            af::deviceInfo(name, platform, toolkit, compute);

            DeviceCacheEntry deviceCacheEntry =
            {
                name,
                platform,
                toolkit,
                compute,
                af::getMemStepSize(),

                backend,
                devIndex
            };

            // ArrayFire only returns the vendor for CPU entry names, so if
            // we support it, replace this with the full name.
            if((::AF_BACKEND_CPU == backend) && isCPUIDSupported())
            {
                deviceCacheEntry.name = getProcessorName();
            }

            // Policy: some devices are supported by multiple backends. Only
            //         store each device once, with the most efficient backend
            //         that supports it.
            auto devIter = std::find_if(
                               deviceCache.begin(),
                               deviceCache.end(),
                               [&deviceCacheEntry](const DeviceCacheEntry& entry)
                               {
                                   return (deviceCacheEntry.name == entry.name);
                               });
            if(deviceCache.end() == devIter)
            {            
                if(af::isDoubleAvailable(devIndex))
                {
                    deviceCache.emplace_back(std::move(deviceCacheEntry));
                }
                else
                {
                    auto& logger = Poco::Logger::get("PothosArrayFire");
                    poco_warning_f2(
                        logger,
                        "Found %s device %s, which does not have 64-bit floating-point "
                        "support through ArrayFire. This device will not be made "
                        "available through PothosArrayFire.",
                        Pothos::Object(backend).convert<std::string>(),
                        deviceCacheEntry.name);
                }
            }
        }
    }

    return deviceCache;
}

const std::vector<af::Backend>& getAvailableBackends()
{
    // Only do this once
    static const std::vector<af::Backend> availableBackends = _getAvailableBackends();

    return availableBackends;
}

const std::vector<DeviceCacheEntry>& getDeviceCache()
{
    // Only do this once
    static const std::vector<DeviceCacheEntry> deviceCache = _getDeviceCache();

    return deviceCache;
}

// Force device caching on init
pothos_static_block(arrayFireCacheDevices)
{
#if !IS_AF_CONFIG_PER_THREAD
    // Set the global backend and device on init.
    // TODO: smarter logic to use most powerful device
    af::setBackend(getAvailableBackends()[0]);
    af::setDevice(0);
#endif
}

//
// Managed interface to device cache
//

// Don't expose a constructor since this should never be used outside
// getting the cache.
static auto managedDeviceCacheEntry = Pothos::ManagedClass()
    .registerClass<DeviceCacheEntry>()
    .registerField("Name", &DeviceCacheEntry::name)
    .registerField("Platform", &DeviceCacheEntry::platform)
    .registerField("Toolkit", &DeviceCacheEntry::toolkit)
    .registerField("Compute", &DeviceCacheEntry::compute)
    .registerField("Memory Step Size", &DeviceCacheEntry::memoryStepSize)
    .commit("ArrayFire/DeviceCacheEntry");

// Nicer than the error from at()
static DeviceCacheEntry getEntry(const DeviceCache& deviceCache, size_t index)
{
    if(index >= deviceCache.size())
    {
        throw Pothos::InvalidArgumentException("Invalid index", std::to_string(index));
    }

    return deviceCache[index];
}

// The constructor will return the full list.
static DeviceCache deviceCacheCtor()
{
    return getDeviceCache();
}

static auto managedDeviceCache = Pothos::ManagedClass()
    .registerClass<DeviceCache>()
    .registerConstructor(&deviceCacheCtor)
    .registerMethod("getEntry", &getEntry)
    .registerMethod(POTHOS_FCN_TUPLE(DeviceCache, size))
    .commit("ArrayFire/DeviceCache");
