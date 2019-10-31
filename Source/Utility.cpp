// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <cassert>

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

void validateComplexAndFloatTypesMatch(
    const Pothos::DType& complexDType,
    const Pothos::DType& floatDType)
{
    assert(!complexDType.isInteger());
    assert(complexDType.isComplex());
    assert(!floatDType.isInteger());
    assert(!floatDType.isComplex());

    static constexpr size_t subStringStart = 8; // Cut off "complex_"

    auto complexDTypeSubtype = complexDType.name().substr(subStringStart);
    auto floatType = floatDType.name();

    if(complexDTypeSubtype != floatType)
    {
        throw Pothos::InvalidArgumentException(
                  "Incompatible types",
                  (complexDTypeSubtype + ", " + floatType));
    }
}
