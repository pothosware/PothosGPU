// Copyright (c) 2020-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

using namespace PothosGPU;

// To avoid collisions
namespace
{

class CorrCoefBlock: public ArrayFireBlock
{
    public:
        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype)
        {
            static const DTypeSupport dtypeSupport{true,true,true,false};
            validateDType(dtype, dtypeSupport);

            return new CorrCoefBlock(device, dtype);
        }

        CorrCoefBlock(
            const std::string& device,
            const Pothos::DType& dtype)
        :
            ArrayFireBlock(device)
        {
            for(size_t i = 0; i < 2; ++i)
            {
                this->setupInput(i, dtype, _domain);
                this->setupOutput(i, dtype, _domain);
            }

            this->registerCall(this, POTHOS_FCN_TUPLE(CorrCoefBlock, lastValue));
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

            _lastValue = af::corrcoef<double>(afInput0, afInput1);

            this->produceFromAfArray(0, afInput0);
            this->produceFromAfArray(1, afInput1);
        }

        double lastValue() const
        {
            return _lastValue;
        }

    private:

        double _lastValue;
};


//
// Block registries
//

/*
 * |PothosDoc Correlate (GPU)
 *
 * Uses <b>af::corrcoef</b> to calculate the correlation coefficents of two
 * input streams. The last calculated value can be queried with the <b>lastValue</b>
 * probe.
 *
 * |category /GPU/Statistics
 * |keywords array coefficient
 * |factory /gpu/statistics/corrcoef(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerStatisticsCorrCoef(
    "/gpu/statistics/corrcoef",
    Pothos::Callable(&CorrCoefBlock::make));

}
