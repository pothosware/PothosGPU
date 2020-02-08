// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <arrayfire.h>

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

const std::vector<af::Backend>& getAvailableBackends();

const std::vector<DeviceCacheEntry>& getDeviceCache();
