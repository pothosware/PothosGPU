// Copyright (c) 2019-2020 Nicholas Corgan
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
            ArrayFireBlock(device),
            _nchans(numChannels),
            _afDType(Pothos::Object(dtype).convert<af::dtype>())
        {
            for(size_t chan = 0; chan < _nchans; ++chan)
            {
                this->setupInput(chan, dtype);
            }
            this->setupOutput(0, dtype);
        }

        void work() override
        {
            if(0 == this->workInfo().minElements)
            {
                return;
            }

            af::array afInput(static_cast<dim_t>(_nchans), this->workInfo().minElements, _afDType);
            for(dim_t chan = 0; chan < static_cast<dim_t>(_nchans); ++chan)
            {
                afInput.row(chan) = this->getInputPortAsAfArray(chan);
            }

            auto afOutput = af::flat(afInput.T());
            if(1 != afOutput.numdims())
            {
                throw Pothos::AssertionViolationException(
                          "Output of af::flat is not a 1D array",
                          Poco::format(
                              "# dimensions: %s",
                              Poco::NumberFormatter::format(afOutput.numdims())));
            }

            this->postAfArray(0, afOutput);
        }

    private:
        size_t _nchans;
        af::dtype _afDType;
};

/*
 * |PothosDoc Flat
 *
 * Calls <b>af::flat</b> on all inputs, resulting in a single concatenated
 * output.
 *
 * |category /GPU/Data
 * |keywords array data flat
 * |factory /gpu/data/flat(device,dtype,numChannels)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 * |widget ComboBox(editable=false)
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param numChannels[Num Channels] The number of input channels.
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview disable
 */
static Pothos::BlockRegistry registerFlat(
    "/gpu/data/flat",
    Pothos::Callable(&FlatBlock::make));
