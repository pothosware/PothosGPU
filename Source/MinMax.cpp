// Copyright (c) 2019 Nicholas Corgan
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
            const MinMaxFunction& func,
            const Pothos::DType& dtype,
            const std::string& labelName,
            size_t nchans
        ):
            ArrayFireBlock(),
            _dtype(dtype),
            _afDType(Pothos::Object(dtype).convert<af::dtype>()),
            _func(func),
            _labelName(labelName),
            _nchans(nchans)
        {
            for(size_t chan = 0; chan < nchans; ++chan)
            {
                this->setupInput(chan, _dtype);
                this->setupOutput(chan, _dtype, this->getPortDomain());
            }
        }

        virtual ~MinMax() {}

        void work() override
        {
            const size_t elems = this->workInfo().minAllElements;
            if(0 == elems)
            {
                return;
            }

            af::array val, idx;

            auto afInput = this->getNumberedInputPortsAs2DAfArray();
            _func(val, idx, afInput, -1);
            assert(_nchans == static_cast<size_t>(val.elements()));
            assert(_nchans == static_cast<size_t>(idx.elements()));

            const auto* idxPtr = idx.device<std::uint32_t>();

            for(dim_t chan = 0; chan < static_cast<dim_t>(_nchans); ++chan)
            {
                this->output(chan)->postLabel(
                    _labelName,
                    getArrayValueOfUnknownTypeAtIndex(val, chan),
                    idxPtr[chan]);
            }
            this->post2DAfArrayToNumberedOutputPorts(afInput);
        }

    private:

        Pothos::DType _dtype;
        af::dtype _afDType;

        MinMaxFunction _func;
        std::string _labelName;
        size_t _nchans;
};

template <bool isMin>
static Pothos::Block* minMaxFactory(
    const Pothos::DType& dtype,
    size_t nchans)
{
    static constexpr const char* labelName = isMin ? "MIN" : "MAX";

    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new MinMax( \
                           (isMin ? (MinMaxFunction)af::min : (MinMaxFunction)af::max), \
                           dtype, \
                           labelName, \
                           nchans);

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
 * Calls <b>af::min</b> on all inputs. This block computes all outputs
 * in parallel, using one of the following implementations by priority
 * (based on availability of hardware and underlying libraries).
 * <ol>
 * <li>CUDA (if GPU present)</li>
 * <li>OpenCL (if GPU present)</li>
 * <li>Standard C++ (if no GPU present)</li>
 * </ol>
 *
 * For each output, this block posts a <b>"MIN"</b> label, whose position
 * and value match the element of the minimum value.
 *
 * |category /ArrayFire/Algorithm
 * |keywords algorithm min
 * |factory /arrayfire/algorithm/min(dtype,nchans)
 *
 * |param dtype(Data Type) The block data type.
 * |widget DTypeChooser(int16=1,int32=1,uint8=1,uint16=1,uint32=1,float=1)
 * |default "float64"
 * |preview disable
 *
 * |param nchans[Num Channels] The number of channels.
 * |default 1
 * |widget SpinBox(minimum=1)
 * |preview disable
 */
static Pothos::BlockRegistry registerMin(
    "/arrayfire/algorithm/min",
    Pothos::Callable(&minMaxFactory<true>));

/*
 * |PothosDoc Buffer Maximum
 *
 * Calls <b>af::max</b> on all inputs. This block computes all outputs
 * in parallel, using one of the following implementations by priority
 * (based on availability of hardware and underlying libraries).
 * <ol>
 * <li>CUDA (if GPU present)</li>
 * <li>OpenCL (if GPU present)</li>
 * <li>Standard C++ (if no GPU present)</li>
 * </ol>
 *
 * For each output, this block posts a <b>"MAX"</b> label, whose position
 * and value match the element of the maximum value.
 *
 * |category /ArrayFire/Algorithm
 * |keywords algorithm max
 * |factory /arrayfire/algorithm/max(dtype,nchans)
 *
 * |param dtype(Data Type) The block data type.
 * |widget DTypeChooser(int16=1,int32=1,uint8=1,uint16=1,uint32=1,float=1)
 * |default "float64"
 * |preview disable
 *
 * |param nchans[Num Channels] The number of channels.
 * |default 1
 * |widget SpinBox(minimum=1)
 * |preview disable
 */
static Pothos::BlockRegistry registerMax(
    "/arrayfire/algorithm/max",
    Pothos::Callable(&minMaxFactory<false>));
