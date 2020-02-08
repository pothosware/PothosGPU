// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"
#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <iostream>

POTHOS_TEST_BLOCK("/arrayfire/tests", test_managed_device_cache)
{
    // Compare the managed interface to the actual class.
    const auto& nativeDeviceCache = getDeviceCache();

    auto env = Pothos::ProxyEnvironment::make("managed");
    auto deviceCache = env->findProxy("ArrayFire/DeviceCache")();

    // The managed constructor should return the full cache, not
    // an empty vector.
    POTHOS_TEST_EQUAL(
        nativeDeviceCache.size(),
        deviceCache.call<size_t>("size"));

    for(size_t deviceIndex = 0; deviceIndex < nativeDeviceCache.size(); ++deviceIndex)
    {
        auto& nativeDeviceCacheEntry = nativeDeviceCache[deviceIndex];
        auto deviceCacheEntry = deviceCache.call("getEntry", deviceIndex);
        POTHOS_TEST_EQUAL(
            "DeviceCacheEntry",
            deviceCacheEntry.getClassName());

        POTHOS_TEST_EQUAL(
            nativeDeviceCacheEntry.name,
            deviceCacheEntry.get<std::string>("Name"));
        POTHOS_TEST_EQUAL(
            nativeDeviceCacheEntry.platform,
            deviceCacheEntry.get<std::string>("Platform"));
        POTHOS_TEST_EQUAL(
            nativeDeviceCacheEntry.toolkit,
            deviceCacheEntry.get<std::string>("Toolkit"));
        POTHOS_TEST_EQUAL(
            nativeDeviceCacheEntry.compute,
            deviceCacheEntry.get<std::string>("Compute"));
        POTHOS_TEST_EQUAL(
            nativeDeviceCacheEntry.memoryStepSize,
            deviceCacheEntry.get<size_t>("Memory Step Size"));
    }
}
