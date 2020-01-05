// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Plugin.hpp>

#include <arrayfire.h>

#include <string>
#include <unordered_map>

static const std::unordered_map<std::string, af::Backend> BackendEnumMap =
{
    {"CPU",    ::AF_BACKEND_CPU},
    {"CUDA",   ::AF_BACKEND_CUDA},
    {"OpenCL", ::AF_BACKEND_OPENCL},
};

static const std::unordered_map<std::string, af::convMode> ConvModeEnumMap =
{
    {"Default", ::AF_CONV_DEFAULT},
    {"Expand",  ::AF_CONV_EXPAND},
};

static const std::unordered_map<std::string, af::convDomain> ConvDomainEnumMap =
{
    {"Auto",    ::AF_CONV_AUTO},
    {"Spatial", ::AF_CONV_SPATIAL},
    {"Freq",    ::AF_CONV_FREQ}
};

static const std::unordered_map<std::string, af::dtype> DTypeEnumMap =
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

#if AF_API_VERSION_CURRENT >= 34
static const std::unordered_map<std::string, af::randomEngineType> RandomEngineTypeEnumMap =
{
    {"Philox",   ::AF_RANDOM_ENGINE_PHILOX},
    {"Threefry", ::AF_RANDOM_ENGINE_THREEFRY},
    {"Mersenne", ::AF_RANDOM_ENGINE_MERSENNE},
};
#endif

static af::dtype pothosDTypeToAfDType(const Pothos::DType& pothosDType)
{
    return getValForKey(DTypeEnumMap, pothosDType.name());
}

static Pothos::DType afDTypeToPothosDType(af::dtype afDType)
{
    return Pothos::DType(getKeyForVal(DTypeEnumMap, afDType));
}

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
        "std_string_to_af_backend",
        "af_backend_to_std_string");
    registerEnumConversion(
        ConvModeEnumMap,
        "std_string_to_af_convmode",
        "af_convmode_to_std_string");
    registerEnumConversion(
        ConvDomainEnumMap,
        "std_string_to_af_convdomain",
        "af_convdomain_to_std_string");
#if AF_API_VERSION_CURRENT >= 34
    registerEnumConversion(
        RandomEngineTypeEnumMap,
        "std_string_to_af_randomenginetype",
        "af_randomenginetype_to_std_string");
#endif

    // Different enough to not use helper function
    Pothos::PluginRegistry::add(
        "/object/convert/arrayfire/pothos_dtype_to_af_dtype",
        Pothos::Callable(&pothosDTypeToAfDType));
    Pothos::PluginRegistry::add(
        "/object/convert/arrayfire/af_dtype_to_pothos_dtype",
        Pothos::Callable(&afDTypeToPothosDType));
}
