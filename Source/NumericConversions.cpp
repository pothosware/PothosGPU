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

pothos_static_block(registerArrayFireNumericConversions)
{
    registerComplexConversion<std::complex<float>, af::cfloat>("float");
    registerComplexConversion<std::complex<double>, af::cdouble>("double");
}
