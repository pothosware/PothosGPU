// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"

std::vector<DeviceCacheEntry> _getDeviceCache()
{
    std::vector<DeviceCacheEntry> deviceCache;

    int availableBackends = af::getAvailableBackends();
    const std::vector<::af_backend> BACKENDS =
    {
        ::AF_BACKEND_CUDA,
        ::AF_BACKEND_OPENCL,
        ::AF_BACKEND_CPU,
    };
    for(const auto& backend: BACKENDS)
    {
        if(availableBackends & backend)
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

                deviceCache.emplace_back(std::move(deviceCacheEntry));
            }
        }
    }

    return deviceCache;
}

const std::vector<DeviceCacheEntry>& getDeviceCache()
{
    // Only do this once
    static const std::vector<DeviceCacheEntry> deviceCache = _getDeviceCache();

    return deviceCache;
}
