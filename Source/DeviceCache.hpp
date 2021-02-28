// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <arrayfire.h>

#include <Pothos/Config.hpp>

#include <string>
#include <vector>

struct DeviceCacheEntry
{
    std::string name;
    std::string platform;
    std::string toolkit;
    std::string compute;
    size_t memoryStepSize;

    af::Backend afBackendEnum;
    int afDeviceIndex;
};
using DeviceCache = std::vector<DeviceCacheEntry>;

POTHOS_LOCAL const std::vector<af::Backend>& getAvailableBackends();

POTHOS_LOCAL const std::vector<DeviceCacheEntry>& getDeviceCache();

POTHOS_LOCAL std::string getAnyDeviceWithBackend(af::Backend backend);

POTHOS_LOCAL std::string getCPUOrBestDevice();
