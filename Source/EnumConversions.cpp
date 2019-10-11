// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <Pothos/Plugin.hpp>

#include <arrayfire.h>

#include <string>
#include <unordered_map>

static const std::unordered_map<std::string, ::af_backend> BackendEnumMap =
{
    {"CPU",    ::AF_BACKEND_CPU},
    {"CUDA",   ::AF_BACKEND_CUDA},
    {"OpenCL", ::AF_BACKEND_OPENCL},
};

template <typename KeyType, typename ValType, typename HasherType>
static void registerEnumConversion(
    const std::unordered_map<KeyType, ValType, HasherType>& unorderedMap,
    const std::string& keyToValPluginName,
    const std::string& valToKeyPluginName)
{
    static const std::string ConvertPluginSubpath("/object/convert/arrayfire/");

    const std::string keyToValPluginPath = ConvertPluginSubpath + keyToValPluginName;
    const std::string valToKeyPluginPath = ConvertPluginSubpath + valToKeyPluginName;

    Pothos::PluginRegistry::add(
        keyToValPluginPath,
        Pothos::Callable(&getValForKey<KeyType, ValType, HasherType>)
            .bind(unorderedMap, 0));
    Pothos::PluginRegistry::add(
        valToKeyPluginPath,
        Pothos::Callable(&getKeyForVal<KeyType, ValType, HasherType>)
            .bind(unorderedMap, 0));
}

pothos_static_block(registerArrayFireEnumConversions)
{
    registerEnumConversion(
        BackendEnumMap,
        "string_to_af_backend",
        "af_backend_to_string");
}
