// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cstdint>
#include <typeinfo>

using MinMaxFunction = void(*)(af::array&, af::array&, const af::array&, const int);

class MinMax: public ArrayFireBlock
{
    public:

        MinMax(
            const std::string& device,
            const MinMaxFunction& func,
            const Pothos::DType& dtype
        ):
            ArrayFireBlock(device),
            _dtype(dtype),
            _afDType(Pothos::Object(dtype).convert<af::dtype>()),
            _func(func)
        {
            this->setupInput(0, _dtype);
            this->setupOutput(0, _dtype);

            this->registerCall(this, POTHOS_FCN_TUPLE(MinMax, lastValue));
            this->registerProbe("lastValue");
        }

        virtual ~MinMax() {}

        Pothos::Object lastValue() const
        {
            return _lastValue;
        }

        void work() override
        {
            const size_t elems = this->workInfo().minElements;
            if(0 == elems)
            {
                return;
            }

            af::array val, idx;

            auto afInput = this->getInputPortAsAfArray(0);
            _func(val, idx, afInput, -1);

            _lastValue = getArrayValueOfUnknownTypeAtIndex(val, 0);

            this->produceFromAfArray(0, afInput);
        }

    private:

        Pothos::DType _dtype;
        af::dtype _afDType;

        MinMaxFunction _func;
        size_t _nchans;

        Pothos::Object _lastValue;
};

template <bool isMin>
static Pothos::Block* minMaxFactory(
    const std::string& device,
    const Pothos::DType& dtype)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new MinMax( \
                           device, \
                           (isMin ? (MinMaxFunction)af::min : (MinMaxFunction)af::max), \
                           dtype);

    // ArrayFire has no implementation for int8_t, int64_t, or uint64_t.
    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::uint8_t)
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
}

/*
 * |PothosDoc Buffer Minimum
 *
 * Calls <b>af::min</b> on all inputs.
 *
 * The most recent minimum value can be queried using the "lastValue" probe.
 *
 * |category /ArrayFire/Statistics
 * |keywords algorithm min
 * |factory /arrayfire/algorithm/min(device,dtype)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param dtype[Data Type] The block data type.
 * |widget DTypeChooser(int16=1,int32=1,uint8=1,uint16=1,uint32=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerMin(
    "/arrayfire/algorithm/min",
    Pothos::Callable(&minMaxFactory<true>));

/*
 * |PothosDoc Buffer Maximum
 *
 * Calls <b>af::max</b> on all inputs.
 *
 * The most recent minimum value can be queried using the "lastValue" probe.
 *
 * |category /ArrayFire/Statistics
 * |keywords algorithm max
 * |factory /arrayfire/algorithm/max(device,dtype)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param dtype[Data Type] The block data type.
 * |widget DTypeChooser(int16=1,int32=1,uint8=1,uint16=1,uint32=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerMax(
    "/arrayfire/algorithm/max",
    Pothos::Callable(&minMaxFactory<false>));
