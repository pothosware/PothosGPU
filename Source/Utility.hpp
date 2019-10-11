// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Exception.hpp>

#include <algorithm>
#include <sstream>
#include <unordered_map>

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
