// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <functional>
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
            OneArrayStatsFuncPtr func,
            const Pothos::DType& dtype,
            const std::string& labelName,
            size_t nchans)
        {
            return new OneArrayStatsBlock(
                           func,
                           dtype,
                           labelName,
                           nchans);
        }

        OneArrayStatsBlock(
            OneArrayStatsFunction func,
            const Pothos::DType& dtype,
            const std::string& labelName,
            size_t nchans
        ):
            ArrayFireBlock(),
            _func(std::move(func)),
            _dtype(dtype),
            _afDType(Pothos::Object(dtype).convert<af::dtype>()),
            _labelName(labelName),
            _nchans(nchans)
        {
            validateDType(dtype, floatOnlyDTypeSupport);

            for(size_t chan = 0; chan < _nchans; ++chan)
            {
                this->setupInput(chan, _dtype);

                // Custom domain because of buffer forwarding
                this->setupOutput(chan, _dtype, this->uid());
            }
        }

        OneArrayStatsBlock(
            OneArrayStatsFuncPtr func,
            const Pothos::DType& dtype,
            const std::string& labelName,
            size_t nchans
        ):
            OneArrayStatsBlock(
                OneArrayStatsFunction(func),
                dtype,
                labelName,
                nchans)
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

                output->postLabel(
                    _labelName,
                    getArrayIndexOfUnknownType(afLabelValues, chan),
                    0 /*index*/);
                output->postBuffer(std::move(input->takeBuffer()));
            }
        }

    protected:

        OneArrayStatsFunction _func;
        Pothos::DType _dtype;
        af::dtype _afDType;
        std::string _labelName;
        size_t _nchans;
};

class VarianceBlock: public OneArrayStatsBlock
{
    public:

        static Pothos::Block* make(
            const Pothos::DType& dtype,
            bool isBiased,
            size_t nchans)
        {
            return new VarianceBlock(dtype, isBiased, nchans);
        }

        VarianceBlock(
            const Pothos::DType& dtype,
            bool isBiased,
            size_t nchans
        ):
            OneArrayStatsBlock(
                getAfVarBoundFunction(isBiased),
                dtype,
                "VAR",
                nchans),
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
            .bind<OneArrayStatsFuncPtr>(&af::mean, 0)
            .bind<std::string>("MEAN", 2)),
    Pothos::BlockRegistry(
        "/arrayfire/statistics/var",
        Pothos::Callable(&VarianceBlock::make)),
    Pothos::BlockRegistry(
        "/arrayfire/statistics/stdev",
        Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
            .bind<OneArrayStatsFuncPtr>(&af::stdev, 0)
            .bind<std::string>("STDDEV", 2)),
    Pothos::BlockRegistry(
        "/arrayfire/statistics/median",
        Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
            .bind<OneArrayStatsFuncPtr>(&af::median, 0)
            .bind<std::string>("MEDIAN", 2)),
};
