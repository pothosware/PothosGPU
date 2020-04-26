// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <Pothos/Object.hpp>
#include <Pothos/Object/Serialize.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Util/TypeInfo.hpp>

#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include <arrayfire.h>

#include <cstring>
#include <functional>
#include <vector>

//
// Comparison
//

template <typename AfArrType>
static int compareAfArray(const AfArrType& arr0, const AfArrType& arr1)
{
    // We can't do anything if the arrays were generated with different
    // backends.
    if(af::getBackendId(arr0) != af::getBackendId(arr1))
    {
        return (af::getBackendId(arr0) < af::getBackendId(arr1)) ? -1 : 1;
    }

    af::setBackend(af::getBackendId(arr0));

    // Note: the C++ equality operators for ArrayFire types results in
    // another ArrayFire array with the results of a per-element comparison,
    // so we'll do this instead.
    if(arr0.bytes() == arr1.bytes())
    {
        const auto bytes = arr0.bytes();

        std::vector<std::uint8_t> vec0(bytes);
        std::vector<std::uint8_t> vec1(bytes);

        arr0.host(vec0.data());
        arr1.host(vec1.data());

        return std::memcmp(vec0.data(), vec1.data(), bytes);
    }
    else
    {
        return (arr0.bytes() > arr1.bytes()) ? -1 : 1;
    }
}

//
// Serialization
//

namespace Pothos { namespace serialization {

template <class Archive>
void save(Archive& ar, const af::array& afArray, const unsigned int)
{
    // Only for this thread
    af::setBackend(af::getBackendId(afArray));

    std::vector<std::uint8_t> hostVec(afArray.bytes());
    afArray.host(hostVec.data());

    const auto dims = afArray.dims();

    ar << hostVec;
    ar << dims[0];
    ar << dims[1];
    ar << dims[2];
    ar << dims[3];
    ar << static_cast<int>(af::getBackendId(afArray));
    ar << static_cast<int>(afArray.type());
}

template <class Archive>
void load(Archive& ar, af::array& afArray, const unsigned int)
{
    std::vector<std::uint8_t> hostVec;
    int backendInt = 0;
    int typeInt = 0;
    af::dim4 dims;

    ar >> hostVec;
    ar >> dims[3];
    ar >> dims[2];
    ar >> dims[1];
    ar >> dims[0];
    ar >> backendInt;
    ar >> typeInt;

    // Only for this thread.
    af::setBackend(static_cast<af::Backend>(backendInt));

    afArray = af::array(dims, static_cast<af::dtype>(typeInt));
    afArray.write(hostVec.data(), hostVec.size(), ::afHost);
}
}}

POTHOS_SERIALIZATION_SPLIT_FREE(af::array)
POTHOS_OBJECT_SERIALIZE(af::array)

//
// ToString()
//

template <typename T>
static std::string afEnumToString(T input)
{
    return Pothos::Object(input).convert<std::string>();
}

template <typename T>
static void registerEnumToString(const std::string& leafName)
{
    using Func = std::string(*)(const T& input);
    static auto impl = [](const T& input){return Pothos::Object(input).convert<std::string>();};

    Pothos::registerToStringFunc<T>(
        "ArrayFire/"+leafName,
        Pothos::Callable((Func)impl),
        false /*registerPointerTypes*/);
}

template <typename AfArrType>
static std::string afArrayToString(const AfArrType& arr)
{
    std::string output = Poco::format("%s (backend: %s, dtype: %s",
                                      Pothos::Util::typeInfoToString(typeid(arr)),
                                      Pothos::Object(af::getBackendId(arr)).toString(),
                                      Pothos::Object(arr.type()).toString());

    if(0 != arr.numdims())
    {
        output += ", shape: ";
        for(size_t dim = 0; dim < arr.numdims(); ++dim)
        {
            if(dim > 0) output += "x";
            output += std::to_string(arr.dims(dim));
        }
    }
    output += ")";

    return output;
}

//
// Registration
//
pothos_static_block(pothosArrayFireRegisterObjectFunctions)
{
    Pothos::registerToStringFunc<af::array>(
        "ArrayFire/af_array",
        Pothos::Callable(&afArrayToString<af::array>),
        false /*registerPointerTypes*/);
    Pothos::registerToStringFunc<af::array::array_proxy>(
        "ArrayFire/af_array_arrayproxy",
        Pothos::Callable(&afArrayToString<af::array::array_proxy>),
        false /*registerPointerTypes*/);

    registerEnumToString<af::Backend>("af_backend");
    registerEnumToString<af::convMode>("af_convmode");
    registerEnumToString<af::convDomain>("af_convdomain");
    registerEnumToString<af::dtype>("af_dtype");
    registerEnumToString<af::randomEngineType>("af_randomenginetype");
    registerEnumToString<af::topkFunction>("af_topkfunction");

    Pothos::PluginRegistry::addCall(
        "/object/compare/arrayfire/af_array",
        Pothos::Callable(&compareAfArray<af::array>));
}
