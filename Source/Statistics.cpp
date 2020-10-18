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
            const DTypeSupport& dtypeSupport,
            const Pothos::DType& dtype)
        {
            validateDType(dtype, dtypeSupport);

            return new OneArrayStatsBlock(
                           device,
                           func,
                           dtype);
        }

        OneArrayStatsBlock(
            const std::string& device,
            OneArrayStatsFunction func,
            const Pothos::DType& dtype
        ):
            ArrayFireBlock(device),
            _func(std::move(func)),
            _dtype(dtype),
            _afDType(Pothos::Object(dtype).convert<af::dtype>())
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
            const Pothos::DType& dtype
        ):
            OneArrayStatsBlock(
                device,
                OneArrayStatsFunction(func),
                dtype)
        {}

        double lastValue() const
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

            _lastValue = getArrayValueOfUnknownTypeAtIndex(afLabelValues, 0).convert<double>();

            this->produceFromAfArray(0, afArray);
        }

    protected:

        OneArrayStatsFunction _func;
        Pothos::DType _dtype;
        af::dtype _afDType;
        double _lastValue;
};

class VarianceBlock: public OneArrayStatsBlock
{
    public:

        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype,
            bool isBiased)
        {
            validateDType(dtype, floatOnlyDTypeSupport);

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
                dtype),
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

/*
 * |PothosDoc Mean
 *
 * Calls <b>af::mean</b> on each input buffer to calculate the
 * arithmetic mean of the given values. The result of the last calculation
 * can be queried with the <b>lastValue</b> probe.
 *
 * The incoming buffer is forwarded to the output port with no changes.
 *
 * |category /GPU/Statistics
 * |keywords statistics stats mean average
 * |factory /gpu/statistics/mean(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerMean(
    "/gpu/statistics/mean",
    Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
        .bind<OneArrayStatsFuncPtr>(&af::mean, 1)
        .bind<DTypeSupport>(DTypeSupport(floatOnlyDTypeSupport), 2));

/*
 * |PothosDoc Median
 *
 * Calls <b>af::median</b> on each input buffer to calculate the
 * median of the given values. The result of the last calculation
 * can be queried with the <b>lastValue</b> probe.
 *
 * The incoming buffer is forwarded to the output port with no changes.
 *
 * |category /GPU/Statistics
 * |keywords statistics stats
 * |factory /gpu/statistics/median(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerMedian(
    "/gpu/statistics/median",
    Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
        .bind<OneArrayStatsFuncPtr>(&af::median, 1)
        .bind<DTypeSupport>({true,true,true,false}, 2));

/*
 * |PothosDoc RMS
 *
 * Calculates the root mean square (RMS) of the given values.
 * The result of the last calculation can be queried with the
 * <b>lastValue</b> probe.
 *
 * The incoming buffer is forwarded to the output port with no changes.
 *
 * |category /GPU/Statistics
 * |keywords statistics stats root mean square
 * |factory /gpu/statistics/rms(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerRMS(
    "/gpu/statistics/rms",
    Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
        .bind<OneArrayStatsFuncPtr>(&afRMS, 1)
        .bind<DTypeSupport>(DTypeSupport(floatOnlyDTypeSupport), 2));

/*
 * |PothosDoc Variance
 *
 * Calls <b>af::var</b> on each input buffer to calculate the
 * median of the given values. The result of the last calculation
 * can be queried with the <b>lastValue</b> probe.
 *
 * The incoming buffer is forwarded to the output port with no changes.
 *
 * |category /GPU/Statistics
 * |keywords statistics stats root mean square
 * |factory /gpu/statistics/var(device,dtype,isBiased)
 * |setter setIsBiased(isBiased)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param isBiased[Is Biased?] Whether or not the input values contain sample bias.
 * |widget ToggleSwitch(on="True", off="False")
 * |default False
 */
static Pothos::BlockRegistry registerVar(
    "/gpu/statistics/var",
    Pothos::Callable(&VarianceBlock::make));

/*
 * |PothosDoc Standard Deviation
 *
 * Calls <b>af::stdev</b> on each input buffer to calculate the
 * standard deviation of the given values. The result of the last calculation
 * can be queried with the <b>lastValue</b> probe.
 *
 * The incoming buffer is forwarded to the output port with no changes.
 *
 * |category /GPU/Statistics
 * |keywords statistics stats stddev
 * |factory /gpu/statistics/stdev(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerStdev(
    "/gpu/statistics/stdev",
    Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
        .bind<OneArrayStatsFuncPtr>(&af::stdev, 1)
        .bind<DTypeSupport>(DTypeSupport(floatOnlyDTypeSupport), 2));

/*
 * |PothosDoc Median Absolute Deviation
 *
 * Calculates the median absolute deviation of the given values.
 * The result of the last calculation can be queried with the
 * <b>lastValue</b> probe.
 *
 * The incoming buffer is forwarded to the output port with no changes.
 *
 * |category /GPU/Statistics
 * |keywords statistics stats mad
 * |factory /gpu/statistics/medabsdev(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerMedAbsDev(
    "/gpu/statistics/medabsdev",
    Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
        .bind<OneArrayStatsFuncPtr>(&afMedAbsDev, 1)
        .bind<DTypeSupport>(DTypeSupport(floatOnlyDTypeSupport), 2));
