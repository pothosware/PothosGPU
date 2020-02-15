// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>
#include <Poco/String.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

void setThreadAFBackend(af::Backend backend)
{
    // Since the preprocessor makes this a literal, the compiler should
    // optimize away the inapplicable case.
    if(IS_AF_CONFIG_PER_THREAD)
    {
        af::setBackend(backend);
        assert(backend == af::getActiveBackend());
    }
    else
    {
        throw Pothos::RuntimeException(
                  Poco::format(
                      "This build of PothosArrayFire was built against ArrayFire %s, "
                      "which only supports a single global backend. To enable setting "
                      "different backends per block, rebuild this module against "
                      "ArrayFire 3.5+.",
                      std::string(AF_VERSION)));
    }
}

void setThreadAFDevice(const std::string& device)
{
    // Since the preprocessor makes this a literal, the compiler should
    // optimize away the inapplicable case.
    if(IS_AF_CONFIG_PER_THREAD)
    {
        const auto backend = af::getActiveBackend();
        const auto& deviceCache = getDeviceCache();

        // Search for the given device name in our current backend.
        auto deviceIter = std::find_if(
                              deviceCache.begin(),
                              deviceCache.end(),
                              [&backend, &device](const DeviceCacheEntry& entry)
                              {
                                  return (entry.afBackendEnum == backend) &&
                                         (entry.name == device);
                              });
        if(deviceIter != deviceCache.end())
        {
            af::setDevice(deviceIter->afDeviceIndex);
            assert(deviceIter->afDeviceIndex == af::getDevice());
        }
        else
        {
            throw Pothos::NotFoundException(
                      Poco::format(
                          "Could not find device with backend %s and name %s.",
                          Pothos::Object(backend).convert<std::string>(),
                          device));
        }
    }
    else
    {
        throw Pothos::RuntimeException(
                  Poco::format(
                      "This build of PothosArrayFire was built against ArrayFire %s, "
                      "which only supports a single global device. To enable setting "
                      "different devices per block, rebuild this module against "
                      "ArrayFire 3.5+.",
                      std::string(AF_VERSION)));
    }
}

void validateDType(
    const Pothos::DType& dtype,
    const DTypeSupport& supportedTypes)
{
    // Make sure *something* is supported.
    assert(supportedTypes.supportInt ||
           supportedTypes.supportUInt ||
           supportedTypes.supportFloat ||
           supportedTypes.supportComplexFloat);

    // Specific error for types not supported by any block
    static const std::vector<std::string> globalUnsupportedTypes =
    {
        "int8",
        "complex_int8",
        "complex_int16",
        "complex_int32",
        "complex_int64",
        "complex_uint8",
        "complex_uint16",
        "complex_uint32",
        "complex_uint64",
    };
    if(doesVectorContainValue(globalUnsupportedTypes, dtype.name()))
    {
        throw Pothos::InvalidArgumentException(
                  "PothosArrayFire blocks do not support this type",
                  dtype.name());
    }

    const bool isDTypeSupported = (isDTypeInt(dtype) && supportedTypes.supportInt) ||
                                  (isDTypeUInt(dtype) && supportedTypes.supportUInt) ||
                                  (isDTypeFloat(dtype) && supportedTypes.supportFloat) ||
                                  (isDTypeComplexFloat(dtype) && supportedTypes.supportComplexFloat);

    if(!isDTypeSupported)
    {
        throw Pothos::InvalidArgumentException(
                  "Unsupported type",
                  dtype.name());
    }
}

Pothos::Object getArrayValueOfUnknownTypeAtIndex(
    const af::array& afArray,
    dim_t index)
{
    const auto& arrIndex = afArray(index);
    Pothos::Object ret;

    assert(arrIndex.elements() == 1);

    #define SwitchCase(afDType, ctype) \
        case afDType: \
            ret = Pothos::Object(arrIndex.scalar<PothosToAF<ctype>::type>()).convert(typeid(ctype)); \
            break;

    switch(afArray.type())
    {
        SwitchCase(::s16, std::int16_t)
        SwitchCase(::s32, std::int32_t)
        SwitchCase(::s64, long long)
        SwitchCase(::u8,  std::uint8_t)
        SwitchCase(::u16, std::uint16_t)
        SwitchCase(::u32, std::uint32_t)
        SwitchCase(::u64, unsigned long long)
        SwitchCase(::f32, float)
        SwitchCase(::f64, double)
        SwitchCase(::c32, std::complex<float>)
        SwitchCase(::c64, std::complex<double>)

        default:
            throw Pothos::AssertionViolationException("Invalid dtype");
            break;
    }
    #undef SwitchCase

    return ret;
}

ssize_t findValueOfUnknownTypeInArray(
    const af::array& afArray,
    const Pothos::Object& value)
{
    #define SwitchCase(afDType, ctype) \
        case afDType: \
        { \
            const size_t size = static_cast<size_t>(afArray.elements()); \
            const ctype* buffer = reinterpret_cast<const ctype*>(afArray.host<PothosToAF<ctype>::type>()); \
            auto iter = std::find(buffer, (buffer+size), value.extract<ctype>()); \
            af::freeHost(buffer); \
            if(iter != (buffer+size)) \
            { \
                return static_cast<ssize_t>(std::distance(buffer, iter)); \
            } \
            break; \
        }

    switch(afArray.type())
    {
        SwitchCase(::s16, std::int16_t)
        SwitchCase(::s32, std::int32_t)
        SwitchCase(::s64, long long)
        SwitchCase(::u8,  std::uint8_t)
        SwitchCase(::u16, std::uint16_t)
        SwitchCase(::u32, std::uint32_t)
        SwitchCase(::u64, unsigned long long)
        SwitchCase(::f32, float)
        SwitchCase(::f64, double)
        SwitchCase(::c32, std::complex<float>)
        SwitchCase(::c64, std::complex<double>)

        default:
            throw Pothos::AssertionViolationException("Invalid dtype");
            break;
    }
    #undef SwitchCase

    return -1;
}

#if defined(__GNUG__) || defined(__clang__) || defined(_MSC_VER)

#if defined(__GNUG__) || defined(__clang__)
#include <cpuid.h>
#elif defined (_MSC_VER)
#include <intrin.h>
#endif

// Based on: https://github.com/culb/cpuid

static void regToString(
    uint32_t reg,
    std::ostringstream& sstr)
{
    auto* cstr = reinterpret_cast<const char*>(&reg);
    sstr << std::string(cstr, 4);
}

static void cpuIDToString(
    uint32_t leaf,
    std::ostringstream& sstr)
{
    uint32_t eax = 0;
    uint32_t ebx = 0;
    uint32_t ecx = 0;
    uint32_t edx = 0;

#if defined(__GNUG__) || defined(__clang__)
	__cpuid(leaf, eax, ebx, ecx, edx);
#elif defined(_MSC_VER)
	int32_t registers[4];
	__cpuid(registers, leaf);
	eax = registers[0];
	ebx = registers[1];
	ecx = registers[2];
	edx = registers[3];
#endif

    regToString(eax, sstr);
    regToString(ebx, sstr);
    regToString(ecx, sstr);
    regToString(edx, sstr);
}

bool isCPUIDSupported() {return true;}

static std::string cleanupProcessorName(const std::string& str)
{
    // Scanning the registers results in odd termination behavior,
    // but strlen gives us what we need.
    auto ret = str;
    ret.resize(strlen(ret.c_str()));

    return ret;
}

std::string getProcessorName()
{
    std::ostringstream sstr;
    cpuIDToString(0x80000002, sstr);
    cpuIDToString(0x80000003, sstr);
    cpuIDToString(0x80000004, sstr);

    return cleanupProcessorName(sstr.str());
}

#else

bool isCPUIDSupported() {return false;}

std::string getProcessorName() {return "";}

#endif
