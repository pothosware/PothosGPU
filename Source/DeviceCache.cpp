// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"
#include "Utility.hpp"

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
