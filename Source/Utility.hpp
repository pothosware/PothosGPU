// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Exception.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#include <algorithm>
#include <complex>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace PothosGPU
{

//
// Useful typedefs
//

template <typename T>
struct IsComplex : std::false_type {};

template <typename T>
struct IsComplex<std::complex<T>> : std::true_type {};

template <typename T, typename U>
using EnableIfComplex = typename std::enable_if<IsComplex<T>::value, U>::type;

template <typename T, typename U>
using EnableIfInteger = typename std::enable_if<!IsComplex<T>::value && !std::is_floating_point<T>::value && !std::is_unsigned<T>::value, U>::type;

template <typename T, typename U>
using EnableIfUnsignedInt = typename std::enable_if<std::is_unsigned<T>::value, U>::type;

template <typename T, typename U>
using EnableIfAnyInt = typename std::enable_if<!IsComplex<T>::value && !std::is_floating_point<T>::value, U>::type;

template <typename T, typename U>
using EnableIfFloat = typename std::enable_if<!IsComplex<T>::value && std::is_floating_point<T>::value, U>::type;

//
// These helper functions will be used for registering enum conversions.
//

template <typename KeyType, typename ValType, typename HasherType>
static ValType getValForKey(
    const std::unordered_map<KeyType, ValType, HasherType>& unorderedMap,
    const KeyType& key)
{
    auto iter = unorderedMap.find(key);
    if(unorderedMap.end() == iter)
    {
        std::ostringstream errorStream;
        errorStream << "Invalid input: " << key;

        throw Pothos::InvalidArgumentException(errorStream.str());
    }

    return iter->second;
}

template <typename KeyType, typename ValType, typename HasherType>
static KeyType getKeyForVal(
    const std::unordered_map<KeyType, ValType, HasherType>& unorderedMap,
    const ValType& value)
{
    using MapPair = typename std::unordered_map<KeyType, ValType, HasherType>::value_type;

    auto iter = std::find_if(
                    unorderedMap.begin(),
                    unorderedMap.end(),
                    [&value](const MapPair& mapPair)
                    {
                        return (mapPair.second == value);
                    });
    if(unorderedMap.end() == iter)
    {
        std::ostringstream errorStream;
        errorStream << "Invalid input: " << value;

        throw Pothos::InvalidArgumentException(errorStream.str());
    }

    return iter->first;
}

//
// Block input validation
//

struct DTypeSupport
{
    bool supportInt;
    bool supportUInt;
    bool supportFloat;
    bool supportComplexFloat;
};

void validateDType(
    const Pothos::DType& dtype,
    const DTypeSupport& supportedTypes);

bool isSupportedFileSinkType(const Pothos::DType& dtype);

//
// Pothos <-> ArrayFire type conversion
//

template <typename T>
struct PothosToAF
{
    using type = T;

    static inline T from(const T& in)
    {
        return in;
    };

    static inline type to(const T& in)
    {
        return in;
    };
};

template <>
struct PothosToAF<std::complex<float>>
{
    using type = af::cfloat;

    static inline std::complex<float> from(const type& in)
    {
        return Pothos::Object(in).convert<std::complex<float>>();
    }

    static inline type to(const std::complex<float>& in)
    {
        return Pothos::Object(in).convert<type>();
    }
};

template <>
struct PothosToAF<std::complex<double>>
{
    using type = af::cdouble;

    static inline std::complex<double> from(const type& in)
    {
        return Pothos::Object(in).convert<std::complex<double>>();
    }

    static inline type to(const std::complex<double>& in)
    {
        return Pothos::Object(in).convert<type>();
    }
};

Pothos::Object getArrayValueOfUnknownTypeAtIndex(
    const af::array& afArray,
    dim_t index);

ssize_t findValueOfUnknownTypeInArray(
    const af::array& afArray,
    const Pothos::Object& value);

af::array getArrayFromSingleElement(
    const af::array& afArray,
    size_t newArraySize);

Pothos::Object afArrayToStdVector(const af::array& afArray);

//
// ArrayFire requires taps to be specific types for different inputs.
//

template <typename T>
struct Tap
{
    using Type = float;
};

template <>
struct Tap<double>
{
    using Type = double;
};

template <>
struct Tap<std::complex<float>>
{
    using Type = std::complex<float>;
};

template <>
struct Tap<std::complex<double>>
{
    using Type = std::complex<double>;
};

//
// Misc
//

// Because std::algorithm stuff is ugly
template <typename T>
static bool doesVectorContainValue(
    const std::vector<T>& vec,
    const T& value)
{
    return (vec.end() != std::find(vec.begin(), vec.end(), value));
}

// Needed because isInteger() returns true for complex integral types
static inline bool isDTypeInt(const Pothos::DType& dtype)
{
    return (dtype.isInteger() && dtype.isSigned() && !dtype.isComplex());
}

// Needed because isInteger() returns true for complex integral types
static inline bool isDTypeUInt(const Pothos::DType& dtype)
{
    return (dtype.isInteger() && !dtype.isSigned() && !dtype.isComplex());
}

// Convenience
static inline bool isDTypeAnyInt(const Pothos::DType& dtype)
{
    return (dtype.isInteger() && !dtype.isComplex());
}

// Needed because isFloat() returns true for complex float types
static inline bool isDTypeFloat(const Pothos::DType& dtype)
{
    return (dtype.isFloat() && !dtype.isComplex());
}

static inline bool isDTypeComplexFloat(const Pothos::DType& dtype)
{
    return (dtype.isFloat() && dtype.isComplex());
}

bool isCPUIDSupported();

std::string getProcessorName();

#if AF_API_VERSION >= 38
constexpr af::varBias getVarBias(bool isBiased)
{
    return isBiased ? ::AF_VARIANCE_SAMPLE : ::AF_VARIANCE_POPULATION;
}
#endif


//
// Formatting
//

template <typename T>
std::string stdVectorToString(const std::vector<T>& vec)
{
    std::ostringstream ostream;
    for(const T& val: vec)
    {
        if(&val != &vec[0])
        {
            ostream << " ";
        }
        ostream << val;
    }

    return ostream.str();
}

}
