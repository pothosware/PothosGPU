// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <numeric>

#if AF_API_VERSION >= 38

static af::array afNot(const af::array& afArray)
{
    return ~afArray;
}

#else

// We should have caught this by this point.
class DTypeAssertionViolationException: public Pothos::AssertionViolationException
{
    public:
        DTypeAssertionViolationException(af::dtype dtype):
            Pothos::AssertionViolationException(
                "We didn't catch an invalid DType: " +
                Pothos::Object(dtype).convert<Pothos::DType>().toString())
        {}

        virtual ~DTypeAssertionViolationException() = default;
};

namespace detail
{
    // https://en.wikipedia.org/wiki/Bitwise_operation#NOT

    template <typename T>
    EnableIfInteger<T, af::array> afNot(const af::array& afArray)
    {
        return -afArray - 1;
    }

    template <typename T>
    EnableIfUnsignedInt<T, af::array> afNot(const af::array& afArray)
    {
        return std::numeric_limits<T>::max() - afArray;
    }
}

static af::array afNot(const af::array& afArray)
{
    switch(afArray.type())
    {
        case s16: return detail::afNot<std::int16_t>(afArray);
        case s32: return detail::afNot<std::int32_t>(afArray);
        case s64: return detail::afNot<std::int64_t>(afArray);
        case u8:  return detail::afNot<std::uint8_t>(afArray);
        case u16: return detail::afNot<std::uint16_t>(afArray);
        case u32: return detail::afNot<std::uint32_t>(afArray);
        case u64: return detail::afNot<std::uint64_t>(afArray);

        default:
            throw DTypeAssertionViolationException(afArray.type());
    }
}

#endif

/*
 * |PothosDoc Bitwise Not
 *
 * Perform the bitwise not operation on all inputs, returning
 * the outputs in the output stream.
 *
 * |category /GPU/Scalar Operations
 * |factory /gpu/scalar/bitwise_not(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,dim=1)
 * |default "uint64"
 * |preview disable
 */
static Pothos::BlockRegistry registerBitwiseNot(
    "/gpu/scalar/bitwise_not",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&afNot, 1)
        .bind<DTypeSupport>({
            true,
            true,
            false,
            false,
        }, 3)
);
