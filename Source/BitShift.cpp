// Copyright (c) 2020-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <Poco/Format.h>

#include <arrayfire.h>

#include <complex>
#include <cstring>
#include <functional>

// To avoid collisions
namespace
{

//
// Utility function getters
//

using OneToOneBlockFcn = std::function<af::array(const af::array&)>;

static OneToOneBlockFcn getLeftShiftFcn(size_t shiftSize)
{
    return [shiftSize](const af::array& afArray)
    {
        return (afArray << shiftSize);
    };
}

static OneToOneBlockFcn getRightShiftFcn(size_t shiftSize)
{
    return [shiftSize](const af::array& afArray)
    {
        return (afArray >> shiftSize);
    };
}

//
// Block class
//

template <typename Type>
class BitShift: public OneToOneBlock
{
public:
    BitShift(
        const std::string& device,
        const Pothos::DType& dtype,
        bool leftShift,
        size_t shiftSize
    ):
        // Callable will be set later
        OneToOneBlock(device, Pothos::Callable(), dtype, dtype),
        _leftShift(leftShift)
    {
        using Class = BitShift<Type>;

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, shiftSize));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setShiftSize));

        this->registerProbe("shiftSize");
        this->registerSignal("shiftSizeChanged");

        // Use caller to validate input and emit signal.
        this->setShiftSize(shiftSize);
    }

    virtual ~BitShift() = default;

    size_t shiftSize() const
    {
        return _shiftSize;
    }

    void setShiftSize(size_t shiftSize)
    {
        if (shiftSize >= (sizeof(Type)*8))
        {
            throw Pothos::RangeException(
                      Poco::format(
                          "Shift size cannot be >= the number of bits (%z) in the type (%s)",
                          (sizeof(Type)*8),
                          Pothos::DType(typeid(Type)).toString()));
        }

        this->_func = _leftShift ? getLeftShiftFcn(shiftSize) : getRightShiftFcn(shiftSize);

        _shiftSize = shiftSize;
        this->emitSignal("shiftSizeChanged", _shiftSize);
    }

private:
    bool _leftShift;
    size_t _shiftSize;
};

//
// Factories
//

class BitwiseParamException: public Pothos::InvalidArgumentException
{
public:
    BitwiseParamException(
        const Pothos::DType& dtype,
        const std::string& operation
    ):
    Pothos::InvalidArgumentException(Poco::format("DType: %s, Operation: %s", dtype.toString(), operation))
    {}
};

static Pothos::Block* makeBitShift(
    const std::string& device,
    const Pothos::DType& dtype,
    const std::string& operation,
    size_t shiftSize)
{
    #define BitShiftFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        { \
            if(operation == "Left Shift")       return new BitShift<T>(device, dtype, true, shiftSize); \
            else if(operation == "Right Shift") return new BitShift<T>(device, dtype, false, shiftSize); \
        }
    
    BitShiftFactory(char)
    else BitShiftFactory(short)
    else BitShiftFactory(int)
    else BitShiftFactory(unsigned char)
    else BitShiftFactory(unsigned short)
    else BitShiftFactory(unsigned)
    else BitShiftFactory(unsigned long long)

    throw BitwiseParamException(dtype, operation);
}

//
// Registration
//

/***********************************************************************
 * |PothosDoc Bit Shift (GPU)
 *
 * Perform a bitwise operation on the given input buffer.
 *
 * |category /GPU/Scalar Operations
 * |keywords left right
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The block data type.
 * |widget DTypeChooser(int8=1,int16=1,int32=1,uint=1,dim=1)
 * |default "uint64"
 * |preview disable
 *
 * |param operation The bit shift operation to perform.
 * |default "Left Shift"
 * |option [Left Shift] "Left Shift"
 * |option [Right Shift] "Right Shift"
 * |preview enable
 *
 * |param shiftSize[Shift Size] The number of bits to shift.
 * |widget SpinBox(minimum=0)
 * |default 0
 * |preview enable
 *
 * |factory /gpu/scalar/bitshift(device,dtype,operation,shiftSize)
 * |setter setShiftSize(shiftSize)
 **********************************************************************/
static Pothos::BlockRegistry registerBitShift(
    "/gpu/scalar/bitshift",
    Pothos::Callable(&makeBitShift));

}
