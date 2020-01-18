// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>

#include <arrayfire.h>

#include <cstdint>
#include <iostream>
#include <typeinfo>

template <typename T>
class SplitComplex: public ArrayFireBlock
{
    public:
        using ComplexType = std::complex<T>;

        SplitComplex(
            const std::string& device,
            size_t nchans,
            size_t dtypeDimensions
        ):
            ArrayFireBlock(device),
            _nchans(nchans)
        {
            for(size_t chan = 0; chan < _nchans; ++chan)
            {
                this->setupInput(
                    chan,
                    Pothos::DType(typeid(ComplexType), dtypeDimensions));

                this->setupOutput(
                    "re"+std::to_string(chan),
                    Pothos::DType(typeid(T), dtypeDimensions),
                    this->getPortDomain());
                this->setupOutput(
                    "im"+std::to_string(chan),
                    Pothos::DType(typeid(T), dtypeDimensions),
                    this->getPortDomain());
            }
        }

        virtual ~SplitComplex() = default;

        void work() override
        {
            const size_t elems = this->workInfo().minElements;
            if(0 == elems)
            {
                return;
            }

            af::array afReal;
            af::array afImag;

            if(1 == _nchans)
            {
                if(this->doesInputPortDomainMatch(0))
                {
                    const auto& afInput = this->getInputPortAfArrayRef(0);
                    afReal = af::real(afInput);
                    afImag = af::imag(afInput);
                }
                else
                {
                    auto afInput = this->getInputPortAsAfArray(0);
                    afReal = af::real(afInput);
                    afImag = af::imag(afInput);
                }
            }
            else
            {
                auto afInput = this->getNumberedInputPortsAs2DAfArray();
                afReal = af::real(afInput);
                afImag = af::imag(afInput);
            }

            if(1 == _nchans)
            {
                this->input(0)->consume(elems);
                this->postAfArray("re0", afReal);
                this->postAfArray("im0", afImag);
            }
            else
            {
                for(size_t chan = 0; chan < this->_nchans; ++chan)
                {
                    this->postAfArray(
                        "re"+std::to_string(chan),
                        afReal.row(chan));
                    this->postAfArray(
                        "im"+std::to_string(chan),
                        afImag.row(chan));
                }
            }
        }

    private:
        size_t _nchans;
};

static Pothos::Block* splitComplexFactory(
    const std::string& device,
    const Pothos::DType& dtype,
    size_t nchans)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new SplitComplex<T>(device,nchans,dtype.dimension());

    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
}

/*
 * |PothosDoc Split Complex (Parallel)
 *
 * Calls <b>af::real</b> and <b>af::imag</b> on all inputs and outputs results
 * in "reX" and "imX" output channels. This is potentially accelerated using
 * one of the following implementations by priority (based on availability of
 * hardware and underlying libraries).
 * <ol>
 * <li>CUDA (if GPU present)</li>
 * <li>OpenCL (if GPU present)</li>
 * <li>Standard C++ (if no GPU present)</li>
 * </ol>
 *
 * |category /ArrayFire/Arith
 * |keywords arith complex real imag imaginary
 * |factory /arrayfire/comms/split_complex(device,dtype,numInputs)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 * |widget ComboBox(editable=false)
 * |preview enable
 *
 * |param dtype(Data Type) The block data type.
 * |widget DTypeChooser(float=1)
 * |default "float64"
 * |preview disable
 *
 * |param numInputs[Num Inputs] The number of input channels.
 * |default 1
 * |widget SpinBox(minimum=1)
 * |preview disable
 */
static Pothos::BlockRegistry registerSplitComplex(
    "/arrayfire/comms/split_complex",
    Pothos::Callable(&splitComplexFactory));
