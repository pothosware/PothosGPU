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
        case b8: return detail::afNot<char>(afArray);
        case s16: return detail::afNot<short>(afArray);
        case s32: return detail::afNot<int>(afArray);
        case s64: return detail::afNot<long long>(afArray);
        case u8:  return detail::afNot<unsigned char>(afArray);
        case u16: return detail::afNot<unsigned short>(afArray);
        case u32: return detail::afNot<unsigned>(afArray);
        case u64: return detail::afNot<unsigned long long>(afArray);

        default:
            throw DTypeAssertionViolationException(afArray.type());
    }
}

#endif

/*
 * |PothosDoc Bitwise Not (GPU)
 *
 * Perform the bitwise not operation on all inputs, returning
 * the outputs in the output stream.
 *
 * |category /GPU/Array Operations
 * |factory /gpu/array/bitwise_not(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int=1,uint=1,dim=1)
 * |default "uint64"
 * |preview disable
 */
static Pothos::BlockRegistry registerBitwiseNot(
    "/gpu/array/bitwise_not",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&afNot, 1)
        .bind<DTypeSupport>({
            true,
            true,
            false,
            false,
        }, 3)
);
