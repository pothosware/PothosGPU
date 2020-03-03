// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

class CovarianceBlock: public ArrayFireBlock
{
    public:
        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype)
        {
            static const DTypeSupport dtypeSupport{true,true,true,false};
            validateDType(dtype, dtypeSupport);

            return new CovarianceBlock(device, dtype);
        }

        CovarianceBlock(
            const std::string& device,
            const Pothos::DType& dtype)
        :
            ArrayFireBlock(device),
            _isBiased(false)
        {
            for(size_t i = 0; i < 2; ++i)
            {
                this->setupInput(i, dtype);
                this->setupOutput(i, dtype);
            }

            this->registerCall(this, POTHOS_FCN_TUPLE(CovarianceBlock, isBiased));
            this->registerCall(this, POTHOS_FCN_TUPLE(CovarianceBlock, setIsBiased));
            this->registerProbe("isBiased");
            this->registerSignal("isBiasedChanged");

            this->registerCall(this, POTHOS_FCN_TUPLE(CovarianceBlock, lastValue));
            this->registerProbe("lastValue");
        }

        void work() override
        {
            const auto elems = this->workInfo().minAllElements;
            if(0 == elems)
            {
                return;
            }

            auto afInput0 = this->getInputPortAsAfArray(0);
            auto afInput1 = this->getInputPortAsAfArray(1);

            auto afLastValue = af::cov(afInput0, afInput1, _isBiased);
            if(1 != afLastValue.elements())
            {
                throw Pothos::AssertionViolationException(
                          "afLastValue: invalid size",
                          std::to_string(afLastValue.elements()));
            }
            _lastValue = getArrayValueOfUnknownTypeAtIndex(afLastValue, 0).convert<double>();

            this->produceFromAfArray(0, afInput0);
            this->produceFromAfArray(1, afInput1);
        }

        bool isBiased() const
        {
            return _isBiased;
        }

        void setIsBiased(bool isBiased)
        {
            _isBiased = isBiased;
            this->emitSignal("isBiasedChanged", _isBiased);
        }

        double lastValue() const
        {
            return _lastValue;
        }

    private:

        double _lastValue;
        bool _isBiased;
};


//
// Block registries
//

/*
 * |PothosDoc Covariance
 *
 * Uses <b>af::cov</b> to calculate the covariance between two input streams. The
 * last calculated value can be queried with the <b>lastValue</b> probe. The input
 * buffers are forwarded unchanged.
 *
 * |category /ArrayFire/Statistics
 * |keywords array coefficient
 * |factory /arrayfire/statistics/cov(device,dtype)
 * |setter setIsBiased(isBiased)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param isBiased[Is Biased?] Whether or not biased estimate should be taken
 * |widget ToggleSwitch(on="True",off="False")
 * |default false
 */
static Pothos::BlockRegistry registerStatisticsCorrCoef(
    "/arrayfire/statistics/cov",
    Pothos::Callable(&CovarianceBlock::make));
