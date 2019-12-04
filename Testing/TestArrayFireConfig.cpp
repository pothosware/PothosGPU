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

// TODO: test behavior when some devices don't support double precision,
// specifically when explicitly setting the backend to one that only has
// a device that doesn't support double precision.
POTHOS_TEST_BLOCK("/arrayfire/tests", test_arrayfire_config)
{
    auto abs = Pothos::BlockRegistry::make(
                   "/arrayfire/arith/abs",
                   Pothos::DType(typeid(float)),
                   3);

    const auto& deviceCache = getDeviceCache();
    POTHOS_TEST_TRUE(!deviceCache.empty());

    for(const auto& entry: deviceCache)
    {
        if(IS_AF_CONFIG_PER_THREAD)
        {
            abs.call("setArrayFireBackend", entry.afBackendEnum);
            abs.call("setArrayFireDevice", entry.name);

            POTHOS_TEST_EQUAL(
                entry.afBackendEnum,
                abs.call<af::Backend>("getArrayFireBackend"));
            POTHOS_TEST_EQUAL(
                entry.name,
                abs.call<std::string>("getArrayFireDevice"));
        }
        else
        {
            POTHOS_TEST_THROWS(
                abs.call("setArrayFireBackend", entry.afBackendEnum)
            , Pothos::RuntimeException);
            POTHOS_TEST_THROWS(
                abs.call("setArrayFireDevice", entry.name)
            , Pothos::RuntimeException);
        }
    }
}
