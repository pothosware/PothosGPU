// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#if AF_API_VERSION >= 38
constexpr af::varBias getVarBias(bool isBiased)
{
    return isBiased ? ::AF_VARIANCE_SAMPLE : ::AF_VARIANCE_POPULATION;
}
#endif

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
#if AF_API_VERSION >= 38
            , _varBias(getVarBias(false))
#endif
        {
            for(size_t i = 0; i < 2; ++i)
            {
                this->setupInput(i, dtype, _domain);
                this->setupOutput(i, dtype, _domain);
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

#if AF_API_VERSION >= 38
            auto afLastValue = af::cov(afInput0, afInput1, _varBias);
#else
            auto afLastValue = af::cov(afInput0, afInput1, _isBiased);
#endif
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

#if AF_API_VERSION >= 38
            _varBias = getVarBias(_isBiased);
#endif
        }

        double lastValue() const
        {
            return _lastValue;
        }

    private:

        double _lastValue;
        bool _isBiased;

#if AF_API_VERSION >= 38
        af::varBias _varBias;
#endif
};


//
// Block registries
//

/*
 * |PothosDoc Covariance (GPU)
 *
 * Uses <b>af::cov</b> to calculate the covariance between two input streams. The
 * last calculated value can be queried with the <b>lastValue</b> probe. The input
 * buffers are forwarded unchanged.
 *
 * |category /GPU/Statistics
 * |keywords array coefficient
 * |factory /gpu/statistics/cov(device,dtype)
 * |setter setIsBiased(isBiased)
 *
 * |param device[Device] Device to use for processing.
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
    "/gpu/statistics/cov",
    Pothos::Callable(&CovarianceBlock::make));
