// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>
#include <Poco/String.h>

#include <arrayfire.h>

#include <functional>
#include <iostream>
#include <typeinfo>
#include <vector>

static const DTypeSupport floatOnlyDTypeSupport = {false,false,true,false};

using OneArrayStatsFuncPtr = af::array(*)(const af::array&, const dim_t);
using OneArrayStatsFunction = std::function<af::array(const af::array&, const dim_t)>;
static constexpr dim_t defaultDim = -1;

static OneArrayStatsFunction getAfVarBoundFunction(bool isBiased)
{
    // af::var is overloaded, so we need this to narrow down to a single function.
    using AfVarFuncPtr = af::array(*)(const af::array&, const bool, const dim_t);

    return std::bind(
               static_cast<AfVarFuncPtr>(af::var),
               std::placeholders::_1,
               isBiased,
               std::placeholders::_2);
}

class OneArrayStatsBlock: public ArrayFireBlock
{
    public:

        static Pothos::Block* makeFromFuncPtr(
            const std::string& device,
            OneArrayStatsFuncPtr func,
            const Pothos::DType& dtype,
            const std::string& labelName,
            size_t nchans,
            bool searchForIndex)
        {
            return new OneArrayStatsBlock(
                           device,
                           func,
                           dtype,
                           labelName,
                           nchans,
                           searchForIndex);
        }

        OneArrayStatsBlock(
            const std::string& device,
            OneArrayStatsFunction func,
            const Pothos::DType& dtype,
            const std::string& labelName,
            size_t nchans,
            bool searchForIndex
        ):
            ArrayFireBlock(device),
            _func(std::move(func)),
            _dtype(dtype),
            _afDType(Pothos::Object(dtype).convert<af::dtype>()),
            _labelName(labelName),
            _nchans(nchans),
            _searchForIndex(searchForIndex)
        {
            validateDType(dtype, floatOnlyDTypeSupport);

            for(size_t chan = 0; chan < _nchans; ++chan)
            {
                this->setupInput(chan, _dtype);

                // Custom domain because of buffer forwarding
                // Note: since we are forwarding the input array,
                //       we aren't setting our own domain.
                this->setupOutput(chan, _dtype, this->uid());
            }
        }

        OneArrayStatsBlock(
            const std::string& device,
            OneArrayStatsFuncPtr func,
            const Pothos::DType& dtype,
            const std::string& labelName,
            size_t nchans,
            bool searchForIndex
        ):
            OneArrayStatsBlock(
                device,
                OneArrayStatsFunction(func),
                dtype,
                labelName,
                nchans,
                searchForIndex)
        {}

        void work() override
        {
            const size_t elems = this->workInfo().minAllElements;
            if(0 == elems)
            {
                return;
            }

            auto afInput = this->getNumberedInputPortsAs2DAfArray();
            auto afLabelValues = _func(afInput, defaultDim);
            assert(afLabelValues.elements() == static_cast<dim_t>(_nchans));

            for(dim_t chan = 0; chan < static_cast<dim_t>(_nchans); ++chan)
            {
                auto* input = this->input(chan);
                auto* output = this->output(chan);

                auto labelVal = getArrayValueOfUnknownTypeAtIndex(afLabelValues, chan);

                size_t index = 0;
                if(_searchForIndex)
                {
                    ssize_t sIndex = findValueOfUnknownTypeInArray(
                                         afInput.row(chan),
                                         labelVal);
                    if(0 > sIndex)
                    {
                        throw Pothos::AssertionViolationException(
                                  Poco::format(
                                      "We couldn't find the %s in the array, "
                                      "but by definition, it should be present.",
                                      Poco::toLower(_labelName)));
                    }

                    index = static_cast<size_t>(sIndex);
                }

                output->postLabel(
                    _labelName,
                    std::move(labelVal),
                    index);
                output->postBuffer(std::move(input->takeBuffer()));
            }
        }

    protected:

        OneArrayStatsFunction _func;
        Pothos::DType _dtype;
        af::dtype _afDType;
        std::string _labelName;
        size_t _nchans;
        bool _searchForIndex;
};

class VarianceBlock: public OneArrayStatsBlock
{
    public:

        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype,
            bool isBiased,
            size_t nchans)
        {
            return new VarianceBlock(device, dtype, isBiased, nchans);
        }

        VarianceBlock(
            const std::string& device,
            const Pothos::DType& dtype,
            bool isBiased,
            size_t nchans
        ):
            OneArrayStatsBlock(
                device,
                getAfVarBoundFunction(isBiased),
                dtype,
                "VAR",
                nchans,
                false /*searchForIndex*/),
            _isBiased(isBiased)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(VarianceBlock, getIsBiased));
            this->registerCall(this, POTHOS_FCN_TUPLE(VarianceBlock, setIsBiased));

            this->registerProbe("isBiased", "isBiasedChanged", "setIsBiased");
            this->emitSignal("isBiasedChanged", isBiased);
        }

        bool getIsBiased() const
        {
            return _isBiased;
        };

        void setIsBiased(bool isBiased)
        {
            _isBiased = isBiased;
            _func = getAfVarBoundFunction(isBiased);

            this->emitSignal("isBiasedChanged", isBiased);
        }

    private:

        bool _isBiased;
};

//
// Factories
//

static const std::vector<Pothos::BlockRegistry> BlockRegistries =
{
    Pothos::BlockRegistry(
        "/arrayfire/statistics/mean",
        Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
            .bind<OneArrayStatsFuncPtr>(&af::mean, 1)
            .bind<std::string>("MEAN", 3)
            .bind<bool>(false /*searchForIndex*/, 5)),
    Pothos::BlockRegistry(
        "/arrayfire/statistics/var",
        Pothos::Callable(&VarianceBlock::make)),
    Pothos::BlockRegistry(
        "/arrayfire/statistics/stdev",
        Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
            .bind<OneArrayStatsFuncPtr>(&af::stdev, 1)
            .bind<std::string>("STDDEV", 3)
            .bind<bool>(false /*searchForIndex*/, 5)),
    Pothos::BlockRegistry(
        "/arrayfire/statistics/median",
        Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
            .bind<OneArrayStatsFuncPtr>(&af::median, 1)
            .bind<std::string>("MEDIAN", 3)
            .bind<bool>(true /*searchForIndex*/, 5)),
};
