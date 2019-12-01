// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"

#include <Pothos/Plugin.hpp>

#include <json.hpp>

#include <arrayfire.h>

#include <Poco/String.h>

#include <algorithm>
#include <iostream>
#include <set>

using json = nlohmann::json;

static json deviceCacheEntryToJSON(const DeviceCacheEntry& entry)
{
    json deviceJSON;
    deviceJSON["Name"] = entry.name;
    deviceJSON["Platform"] = entry.platform;
    deviceJSON["Toolkit"] = entry.toolkit;
    deviceJSON["Compute"] = entry.compute;
    deviceJSON["Memory Step Size"] = entry.memoryStepSize;
    deviceJSON["Float64 supported?"] = entry.supportsDouble;

    return deviceJSON;
}

static std::string _enumerateArrayFireDevices()
{
    json topObject;
    json devicesArray(json::array());
    std::set<std::string> supportedPlatforms;

    const auto& deviceCache = getDeviceCache();
    std::transform(
        std::begin(deviceCache),
        std::end(deviceCache),
        std::back_inserter(devicesArray),
        deviceCacheEntryToJSON);
    std::transform(
        std::begin(deviceCache),
        std::end(deviceCache),
        std::inserter(
            supportedPlatforms,
            std::end(supportedPlatforms)),
        [](const DeviceCacheEntry& entry){return entry.platform;});

    topObject["ArrayFire Device"] = devicesArray;

    auto& arrayFireInfo = topObject["ArrayFire Info"];
    arrayFireInfo["Library Version"] = AF_VERSION;
    arrayFireInfo["API Version"] = AF_API_VERSION;
    arrayFireInfo["Supported Platforms"] = Poco::cat(
                                               std::string(", "),
                                               std::begin(supportedPlatforms),
                                               std::end(supportedPlatforms));

    return topObject.dump();
}

static std::string enumerateArrayFireDevices()
{
    // Only do this once
    static const std::string devs = _enumerateArrayFireDevices();

    return devs;
}

pothos_static_block(registerArrayFireInfo)
{
    Pothos::PluginRegistry::addCall(
        "/devices/arrayfire/info", &enumerateArrayFireDevices);
}
