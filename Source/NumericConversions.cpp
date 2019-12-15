// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Plugin.hpp>

#include <Poco/Format.h>

#include <arrayfire.h>

#include <complex>
#include <string>
#include <unordered_map>
#include <vector>

static const std::string convertPluginSubpath("/object/convert/arrayfire");

//
// std::complex <-> ArrayFire complex
//

template <typename In, typename Out>
static inline Out reinterpretCastEqual(const In& input)
{
    static_assert(sizeof(In) == sizeof(Out));
    return *reinterpret_cast<const Out*>(&input);
}

template <typename StdComplex, typename AFComplex>
static void registerComplexConversion(const std::string& scalarType)
{
    const std::string stdComplexToAFComplexPluginPath =
        Poco::format(
            "%s/std_complex_%s_to_c%s",
            convertPluginSubpath,
            scalarType,
            scalarType);
    const std::string afComplexToStdComplexPluginPath =
        Poco::format(
            "%s/c%s_to_std_complex_%s",
            convertPluginSubpath,
            scalarType,
            scalarType);

    Pothos::PluginRegistry::add(
        stdComplexToAFComplexPluginPath,
        Pothos::Callable(reinterpretCastEqual<StdComplex, AFComplex>));
    Pothos::PluginRegistry::add(
        afComplexToStdComplexPluginPath,
        Pothos::Callable(reinterpretCastEqual<AFComplex, StdComplex>));
}

//
// std::vector <-> af::array
//

template <typename T>
static af::array convertStdVectorToAfArray(const std::vector<T>& vec)
{
    // If this is ever used for types where type and PothosToAF::type are
    // different, we need two versions.
    static_assert(sizeof(T) == sizeof(typename PothosToAF<T>::type));

    return af::array(
               static_cast<dim_t>(vec.size()),
               reinterpret_cast<const typename PothosToAF<T>::type*>(vec.data()),
               ::afHost);
}

template <typename Num, typename Arr>
static std::vector<Num> convertAfArrayToStdVector(const Arr& arr)
{
    assert(Pothos::DType(typeid(Num)) == Pothos::Object(arr.type()).convert<Pothos::DType>());

    std::vector<Num> ret(arr.elements());
    arr.host(ret.data());
    return ret;
}

template <typename T>
static void registerStdVectorConversion(const std::string& typeName)
{
    const std::string stdVectorToAFArrayPluginPath =
        Poco::format(
            "%s/vec%s_to_af_array",
            convertPluginSubpath,
            typeName);
    const std::string afArrayToStdVectorPluginPath =
        Poco::format(
            "%s/af_array_to_vec%s",
            convertPluginSubpath,
            typeName);
    const std::string afArrayProxyToStdVectorPluginPath =
        Poco::format(
            "%s/af_arrayproxy_to_vec%s",
            convertPluginSubpath,
            typeName);

    Pothos::PluginRegistry::add(
        stdVectorToAFArrayPluginPath,
        Pothos::Callable(convertStdVectorToAfArray<T>));
    Pothos::PluginRegistry::add(
        afArrayToStdVectorPluginPath,
        Pothos::Callable(convertAfArrayToStdVector<T,af::array>));
    Pothos::PluginRegistry::add(
        afArrayProxyToStdVectorPluginPath,
        Pothos::Callable(convertAfArrayToStdVector<T,af::array::array_proxy>));
}

pothos_static_block(registerArrayFireNumericConversions)
{
    registerComplexConversion<std::complex<float>, af::cfloat>("float");
    registerComplexConversion<std::complex<double>, af::cdouble>("double");

    registerStdVectorConversion<float>("float");
    registerStdVectorConversion<double>("double");

    registerStdVectorConversion<std::complex<float>>("cfloat");
    registerStdVectorConversion<std::complex<double>>("cdouble");
}
