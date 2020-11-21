// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"
#include "Utility.hpp"

#include <Pothos/Managed.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Plugin.hpp>

#include <Poco/Format.h>
#include <Poco/Logger.h>
#include <Poco/RegularExpression.h>

#include <algorithm>
#include <sstream>

static Poco::Logger& getLogger()
{
    auto& logger = Poco::Logger::get("PothosGPU");
    return logger;
}

template <typename T>
static T castString(const std::string& str)
{
    T ret;
    std::istringstream stream(str);
    stream >> ret;

    return ret;
}

static bool isCUDAVersionValid(const std::string& toolkitStr)
{
    using RE = Poco::RegularExpression;

    bool isValid = false;

    constexpr int options = RE::RE_CASELESS;
    const RE cudaVersionRE("([0-9]+).([0-9]+)", options);

    RE::MatchVec matches;
    const auto numMatches = cudaVersionRE.match(toolkitStr, 0, matches);

    if(3 == numMatches)
    {
        // The first match includes the . between the two numbers, so only
        // look at the other two.
        const auto majorVersionStr = toolkitStr.substr(matches[1].offset, matches[1].length);
        const auto minorVersionStr = toolkitStr.substr(matches[2].offset, matches[2].length);

        const auto majorVersion = castString<size_t>(majorVersionStr);
        const auto minorVersion = castString<size_t>(minorVersionStr);

        const auto versionForComp = (majorVersion * 1000) + minorVersion;

        // 2020/02/18: Currently, the latest CUDA runtime has the crash we're
        // guarding against, so this check will always fail. This should be
        // updated when a CUDA runtime version is released that fixes this.
        // See: https://github.com/gpu/gpu/issues/2707
        constexpr size_t minValidVersion = std::numeric_limits<size_t>::max();

        isValid = (versionForComp >= minValidVersion);
    }
    else
    {
        poco_error_f1(
            getLogger(),
            "Failed to parse CUDA version string %s. Considering invalid for safety.",
            toolkitStr);
    }

    return isValid;
}

static std::vector<af::Backend> _getAvailableBackends()
{
    const std::vector<af::Backend> AllBackends =
    {
        ::AF_BACKEND_CUDA,
        ::AF_BACKEND_OPENCL,
        ::AF_BACKEND_CPU,
    };
    const int afAvailableBackends = af::getAvailableBackends();

    if(0 == afAvailableBackends)
    {
        poco_error(
            getLogger(),
            "No ArrayFire backends detected. Check your ArrayFire installation.");
    }

    std::vector<af::Backend> availableBackends;
    for(const auto& backend: AllBackends)
    {
        if(afAvailableBackends & backend)
        {
            if(::AF_BACKEND_CUDA == backend)
            {
                af::setBackend(backend);
                if(af::getDeviceCount() > 0)
                {
                    static constexpr size_t bufferLen = 1024;
                    char name[bufferLen] = {0};
                    char platform[bufferLen] = {0};
                    char toolkit[bufferLen] = {0};
                    char compute[bufferLen] = {0};
                    af::deviceInfo(name, platform, toolkit, compute);

                    if(isCUDAVersionValid(toolkit))
                    {
                        availableBackends.emplace_back(backend);
                    }
                }
            }
            else
            {
                availableBackends.emplace_back(backend);
            }
        }
    }

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
            if((deviceCache.end() == devIter) && af::isDoubleAvailable(devIndex))
            {
                deviceCache.emplace_back(std::move(deviceCacheEntry));
            }
        }
    }

    if(deviceCache.empty())
    {
        poco_error(
            getLogger(),
            "No ArrayFire devices detected. Check your ArrayFire installation.");
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

std::string getAnyDeviceWithBackend(af::Backend backend)
{
    const auto& deviceCache = getDeviceCache();

    auto deviceIter = std::find_if(
                          deviceCache.begin(),
                          deviceCache.end(),
                          [&backend](const DeviceCacheEntry& entry)
                          {
                              return (entry.afBackendEnum == backend);
                          });
    if(deviceIter == deviceCache.end())
    {
        throw Pothos::Exception(
                  Poco::format(
                      "No devices available with backend %s",
                      Pothos::Object(backend).convert<std::string>()));
    }

    return deviceIter->name;
}

// Force device caching on init
pothos_static_block(arrayFireCacheDevices)
{
    (void)getDeviceCache();
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
    .commit("GPU/DeviceCache");
