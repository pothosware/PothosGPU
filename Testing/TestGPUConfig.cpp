// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"
#include "TestUtility.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <typeinfo>

POTHOS_TEST_BLOCK("/gpu/tests", test_pothosgpu_config)
{
    auto abs = Pothos::BlockRegistry::make(
                   "/gpu/arith/abs",
                   "Auto",
                   Pothos::DType(typeid(float)));

    const auto& deviceCache = PothosGPU::getDeviceCache();
    POTHOS_TEST_FALSE(deviceCache.empty());

    POTHOS_TEST_EQUAL(
        deviceCache[0].afBackendEnum,
        abs.call<af::Backend>("backend"));
    POTHOS_TEST_EQUAL(
        deviceCache[0].name,
        abs.call<std::string>("device"));

    for(const auto& entry: deviceCache)
    {
        abs = Pothos::BlockRegistry::make(
                  "/gpu/arith/abs",
                  entry.name,
                  Pothos::DType(typeid(float)));

        POTHOS_TEST_EQUAL(
            entry.afBackendEnum,
            abs.call<af::Backend>("backend"));
        POTHOS_TEST_EQUAL(
            entry.name,
            abs.call<std::string>("device"));
    }
}
