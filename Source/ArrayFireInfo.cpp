// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <Pothos/Plugin.hpp>

#include <json.hpp>

#include <arrayfire.h>

#include <algorithm>
#include <iostream>

using json = nlohmann::json;

static json getDevicesJSONForBackend(
    ::af_backend backend,
    int& rDevIndex)
{
    json devicesArray(json::array());

    // TODO: lock
    af::setBackend(backend);

    int numDevices = af::getDeviceCount();
    for(int backendDevIndex = 0;
        backendDevIndex < numDevices;
        ++rDevIndex, ++backendDevIndex)
    {
        static constexpr size_t bufferLen = 1024;

        char name[bufferLen] = {0};
        char platform[bufferLen] = {0};
        char toolkit[bufferLen] = {0};
        char compute[bufferLen] = {0};
        af::setDevice(backendDevIndex);
        af::deviceInfo(name, platform, toolkit, compute);

        json topDeviceObject(json::object());
        auto& deviceObject = topDeviceObject[std::to_string(rDevIndex)];
        deviceObject["Name"] = name;
        deviceObject["Platform"] = platform;
        deviceObject["Toolkit"] = toolkit;
        deviceObject["Compute"] = compute;
        deviceObject["Memory Step Size"] = af::getMemStepSize();

        devicesArray.emplace_back(deviceObject);
    }

    return devicesArray;
}

static std::string enumerateArrayFireDevices(void)
{
    json topObject;

    int availableBackends = af::getAvailableBackends();
    static const std::vector<::af_backend> BACKENDS =
    {
        ::AF_BACKEND_CPU,
        ::AF_BACKEND_CUDA,
        ::AF_BACKEND_OPENCL,
    };

    json devicesArray(json::array());
    int devIndex = 0;

    for(const auto& backend: BACKENDS)
    {
        if(availableBackends & backend)
        {
            auto backendDevices = getDevicesJSONForBackend(backend, devIndex);
            if(!backendDevices.empty())
            {
                devicesArray.insert(
                    std::end(devicesArray),
                    std::begin(backendDevices),
                    std::end(backendDevices));
            }
        }
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
