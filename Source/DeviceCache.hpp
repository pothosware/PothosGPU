// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <arrayfire.h>

#include <string>
#include <vector>

namespace PothosGPU
{

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

const std::vector<af::Backend>& getAvailableBackends();

const std::vector<DeviceCacheEntry>& getDeviceCache();

std::string getAnyDeviceWithBackend(af::Backend backend);

std::string getCPUOrBestDevice();

}
