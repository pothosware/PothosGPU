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

static const std::unordered_map<std::string, ::af_dtype> DTypeEnumMap =
{
    // No int8 support
    {"int16",           ::s16},
    {"int32",           ::s32},
    {"int64",           ::s64},
    {"uint8",           ::u8},
    {"uint16",          ::u16},
    {"uint32",          ::u32},
    {"uint64",          ::u64},
    {"float32",         ::f32},
    {"float64",         ::f64},
    // No integral complex support
    {"complex_float32", ::c32},
    {"complex_float64", ::c64},
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
    registerEnumConversion(
        DTypeEnumMap,
        "string_to_af_dtype",
        "af_dtype_to_string");
}
