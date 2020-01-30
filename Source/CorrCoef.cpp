// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

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
                this->setupInput(i, dtype);
                this->setupOutput(i, dtype, this->getPortDomain());
            }
        }

        void work() override
        {
            const auto elems = this->workInfo().minAllInElements;
            if(0 == elems)
            {
                return;
            }

            auto afInput0 = this->getInputPortAsAfArray(0);
            auto afInput1 = this->getInputPortAsAfArray(1);

            const auto corrcoef = af::corrcoef<double>(afInput0, afInput1);

            this->output(0)->postLabel(
                LabelName,
                corrcoef,
                0);
            this->output(1)->postLabel(
                LabelName,
                corrcoef,
                0);

            this->postAfArray(0, std::move(afInput0));
            this->postAfArray(1, std::move(afInput1));
        }

        static const std::string LabelName;
};

const std::string CorrCoefBlock::LabelName = "CORRCOEF";

static Pothos::BlockRegistry registerStatisticsCorrCoef(
    "/arrayfire/statistics/corrcoef",
    Pothos::Callable(&CorrCoefBlock::make));
