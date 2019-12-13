// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"

#include <Pothos/Plugin/Static.hpp>

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
                af::isDoubleAvailable(devIndex),

                backend,
                devIndex
            };

            deviceCache.emplace_back(std::move(deviceCacheEntry));
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
    (void)getAvailableBackends();
    (void)getDeviceCache();
}
