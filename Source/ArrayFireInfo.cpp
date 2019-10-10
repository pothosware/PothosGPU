// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <Pothos/Plugin.hpp>

#include <json.hpp>

#include <arrayfire.h>

using json = nlohmann::json;

static std::string enumerateArrayFireDevices(void)
{
    json topObject;

    // TODO: lock before iterating over devices
    // TODO: iterate over backends, catch error if no devices are available for backend,
    // support all devices
    json devicesArray(json::array());

    int deviceCount = af::getDeviceCount();
    for(int devIndex = 0; devIndex < deviceCount; ++devIndex)
    {
        static constexpr size_t bufferLen = 1024;

        char name[bufferLen] = {0};
        char platform[bufferLen] = {0};
        char toolkit[bufferLen] = {0};
        char compute[bufferLen] = {0};
        af::setDevice(devIndex);
        af::deviceInfo(name, platform, toolkit, compute);

        json topDeviceObject(json::object());
        auto& deviceObject = topDeviceObject[std::to_string(devIndex)];
        deviceObject["Name"] = name;
        deviceObject["Platform"] = platform;
        deviceObject["Toolkit"] = toolkit;
        deviceObject["Compute"] = compute;

        devicesArray.emplace_back(deviceObject);
    }
    if(!devicesArray.empty())
    {
        topObject["ArrayFire Device"] = devicesArray;
    }

    return topObject.dump();
}

pothos_static_block(registerSoapySDRInfo)
{
    Pothos::PluginRegistry::addCall(
        "/devices/arrayfire/info", &enumerateArrayFireDevices);
}
