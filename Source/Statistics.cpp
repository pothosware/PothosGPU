// Copyright (c) 2019-2020 Nicholas Corgan
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

static af::array afMedAbsDev(const af::array& afInput, const dim_t)
{
    af::array afMedian = af::median(afInput);
    af::array afInputSubMedian;

    auto afAbsSub = [](const af::array& arr0, const af::array& arr1) -> af::array
    {
        return af::abs(arr0 - arr1);
    };
    afInputSubMedian = af::batchFunc(afInput, afMedian, afAbsSub);

    return af::median(afInputSubMedian);
}

static af::array afRMS(const af::array& afInput, const dim_t)
{
    const auto arrLen = static_cast<double>(afInput.elements());

    return af::sqrt(af::sum(af::pow(afInput, 2.0)) / arrLen);
}

class OneArrayStatsBlock: public ArrayFireBlock
{
    public:

        static Pothos::Block* makeFromFuncPtr(
            const std::string& device,
            OneArrayStatsFuncPtr func,
            const Pothos::DType& dtype,
            bool searchForIndex)
        {
            return new OneArrayStatsBlock(
                           device,
                           func,
                           dtype,
                           searchForIndex);
        }

        OneArrayStatsBlock(
            const std::string& device,
            OneArrayStatsFunction func,
            const Pothos::DType& dtype,
            bool searchForIndex
        ):
            ArrayFireBlock(device),
            _func(std::move(func)),
            _dtype(dtype),
            _afDType(Pothos::Object(dtype).convert<af::dtype>()),
            _lastValue(),
            _searchForIndex(searchForIndex)
        {
            validateDType(dtype, floatOnlyDTypeSupport);

            this->setupInput(0, _dtype);
            this->setupOutput(0, _dtype);

            this->registerCall(this, POTHOS_FCN_TUPLE(OneArrayStatsBlock, lastValue));
            this->registerProbe("lastValue");
        }

        OneArrayStatsBlock(
            const std::string& device,
            OneArrayStatsFuncPtr func,
            const Pothos::DType& dtype,
            bool searchForIndex
        ):
            OneArrayStatsBlock(
                device,
                OneArrayStatsFunction(func),
                dtype,
                searchForIndex)
        {}

        Pothos::Object lastValue() const
        {
            return _lastValue;
        }

        void work() override
        {
            const size_t elems = this->workInfo().minAllElements;
            if(0 == elems)
            {
                return;
            }

            auto afArray = this->getInputPortAsAfArray(0);
            auto afLabelValues = _func(afArray, defaultDim);
            if(1 != afLabelValues.elements())
            {
                throw Pothos::AssertionViolationException(
                          "afLabelValues: invalid size",
                          std::to_string(afLabelValues.elements()));
            }

            _lastValue = getArrayValueOfUnknownTypeAtIndex(afLabelValues, 0);

            this->postAfArray(0, afArray);
        }

    protected:

        OneArrayStatsFunction _func;
        Pothos::DType _dtype;
        af::dtype _afDType;
        Pothos::Object _lastValue;
        bool _searchForIndex;
};

class VarianceBlock: public OneArrayStatsBlock
{
    public:

        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype,
            bool isBiased)
        {
            return new VarianceBlock(device, dtype, isBiased);
        }

        VarianceBlock(
            const std::string& device,
            const Pothos::DType& dtype,
            bool isBiased
        ):
            OneArrayStatsBlock(
                device,
                getAfVarBoundFunction(isBiased),
                dtype,
                false /*searchForIndex*/),
            _isBiased(isBiased)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(VarianceBlock, isBiased));
            this->registerCall(this, POTHOS_FCN_TUPLE(VarianceBlock, setIsBiased));

            this->registerProbe("isBiased");
            this->registerSignal("isBiasedChanged");

            this->setIsBiased(isBiased);
        }

        bool isBiased() const
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
            .bind<bool>(false /*searchForIndex*/, 3)),
    Pothos::BlockRegistry(
        "/arrayfire/statistics/var",
        Pothos::Callable(&VarianceBlock::make)),
    Pothos::BlockRegistry(
        "/arrayfire/statistics/stdev",
        Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
            .bind<OneArrayStatsFuncPtr>(&af::stdev, 1)
            .bind<bool>(false /*searchForIndex*/, 3)),
    Pothos::BlockRegistry(
        "/arrayfire/statistics/median",
        Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
            .bind<OneArrayStatsFuncPtr>(&af::median, 1)
            .bind<bool>(true /*searchForIndex*/, 3)),
    Pothos::BlockRegistry(
        "/arrayfire/statistics/medabsdev",
        Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
            .bind<OneArrayStatsFuncPtr>(&afMedAbsDev, 1)
            .bind<bool>(false /*searchForIndex*/, 3)),
    Pothos::BlockRegistry(
        "/arrayfire/statistics/rms",
        Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
            .bind<OneArrayStatsFuncPtr>(&afRMS, 1)
            .bind<bool>(false /*searchForIndex*/, 3))
};
