// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include <arrayfire.h>

class FlatBlock: public ArrayFireBlock
{
    public:
        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype,
            size_t numChannels)
        {
            return new FlatBlock(device, dtype, numChannels);
        }

        FlatBlock(
            const std::string& device,
            const Pothos::DType& dtype,
            size_t numChannels
        ):
            ArrayFireBlock(device)
        {
            for(size_t chan = 0; chan < numChannels; ++chan)
            {
                this->setupInput(chan, dtype);
            }
            this->setupOutput(0, dtype, this->getPortDomain());
        }

        void work() override
        {
            if(0 == this->workInfo().minElements)
            {
                return;
            }

            auto afInput = this->getNumberedInputPortsAs2DAfArray();
            auto afOutput = af::flat(afInput.T());
            if(1 != afOutput.numdims())
            {
                throw Pothos::AssertionViolationException(
                          "Output of af::flat is not a 1D array",
                          Poco::format(
                              "# dimensions: %s",
                              Poco::NumberFormatter::format(afOutput.numdims())));
            }

            this->postAfArray(0, std::move(afOutput));
        }
};

/*
 * |PothosDoc Flat
 *
 * Calls <b>af::flat</b> on all inputs, resulting in a single concatenated
 * output.
 *
 * This block uses one of the following implementations by priority (based
 * on availability of hardware and underlying libraries).
 * <ol>
 * <li>CUDA (if GPU present)</li>
 * <li>OpenCL (if GPU present)</li>
 * <li>Standard C++ (if no GPU present)</li>
 * </ol>
 *
 * |category /ArrayFire/Data
 * |keywords array data flat
 * |factory /arrayfire/data/flat(device,dtype,numChannels)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 * |widget ComboBox(editable=false)
 * |preview enable
 *
 * |param dtype(Data Type) The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1)
 * |default "float64"
 * |preview disable
 *
 * |param numChannels(Num Channels) The number of input channels.
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview disable
 */
static Pothos::BlockRegistry registerFlat(
    "/arrayfire/data/flat",
    Pothos::Callable(&FlatBlock::make));
