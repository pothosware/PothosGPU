// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"
#include "TestUtility.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <typeinfo>

POTHOS_TEST_BLOCK("/arrayfire/tests", test_arrayfire_config)
{
    auto abs = Pothos::BlockRegistry::make(
                   "/arrayfire/arith/abs",
                   "Auto",
                   Pothos::DType(typeid(float)),
                   3);

    const auto& deviceCache = getDeviceCache();
    POTHOS_TEST_TRUE(!deviceCache.empty());

    POTHOS_TEST_EQUAL(
        deviceCache[0].afBackendEnum,
        abs.call<af::Backend>("getArrayFireBackend"));
    POTHOS_TEST_EQUAL(
        deviceCache[0].name,
        abs.call<std::string>("getArrayFireDevice"));

    for(const auto& entry: deviceCache)
    {
        abs = Pothos::BlockRegistry::make(
                  "/arrayfire/arith/abs",
                  entry.name,
                  Pothos::DType(typeid(float)),
                  3);

        POTHOS_TEST_EQUAL(
            entry.afBackendEnum,
            abs.call<af::Backend>("getArrayFireBackend"));
        POTHOS_TEST_EQUAL(
            entry.name,
            abs.call<std::string>("getArrayFireDevice"));
    }
}
