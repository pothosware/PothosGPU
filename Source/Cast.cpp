// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Logger.h>

#include <arrayfire.h>

#include <cassert>
#include <cstring>
#include <string>
#include <typeinfo>

static void validateCastTypes(
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType)
{
    if(isDTypeComplexFloat(inputDType) && !outputDType.isComplex())
    {
        throw Pothos::InvalidArgumentException(
                  "This block cannot perform complex to scalar conversions.");
    }
}

class CastBlock: public OneToOneBlock
{
    public:

        // This is what will be registered.
        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            size_t nchans)
        {
            // Validate here to avoid the ArrayFireBlock overhead.
            validateCastTypes(inputDType, outputDType);

            return new CastBlock(device, inputDType, outputDType, nchans);
        }

        CastBlock(
            const std::string& device,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            size_t nchans
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(),
                inputDType,
                outputDType,
                nchans)
        {
        }

        void work(const af::array& afArray) override
        {
            const size_t elems = this->workInfo().minElements;
            assert(elems > 0);

            auto afOutput = afArray.as(_afOutputDType);
            if(1 == _nchans)
            {
                this->input(0)->consume(elems);
                this->output(0)->postBuffer(Pothos::Object(afOutput)
                                                .convert<Pothos::BufferChunk>());
            }
            else
            {
                assert(0 != _nchans);

                this->post2DAfArrayToNumberedOutputPorts(afOutput);
            }
        }
};

/*
 * |PothosDoc Cast
 *
 * Calls <b>af::array::as</b> on all inputs to cast to a given type. This
 * is potentially accelerated using one of the following implementations
 * by priority (based on availability of hardware and underlying libraries).
 * <ol>
 * <li>CUDA (if GPU present)</li>
 * <li>OpenCL (if GPU present)</li>
 * <li>Standard C++ (if no GPU present)</li>
 * </ol>
 *
 * |category /ArrayFire/Stream
 * |keywords array cast
 * |factory /arrayfire/array/cast(device,inputDType,outputDType,numChannels)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 * |widget ComboBox(editable=false)
 * |preview enable
 *
 * |param inputDType(Input Data Type) The block data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1)
 * |default "float64"
 * |preview disable
 *
 * |param outputDType(Output Data Type) The block data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param numChannels[Num Channels] The number of channels.
 * |default 1
 * |widget SpinBox(minimum=1)
 * |preview disable
 */
static Pothos::BlockRegistry registerCast(
    "/arrayfire/array/cast",
    Pothos::Callable(&CastBlock::make));
