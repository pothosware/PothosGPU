// Copyright (c) 2019-2021 Nicholas Corgan
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

using namespace PothosGPU;

// To avoid collisions
namespace
{

//
// Utility
//

static const DTypeSupport floatOnlyDTypeSupport = {false,false,true,false};
static constexpr dim_t defaultDim = -1;

// Matches pre-3.8 signature, we bind the new enum to match
using OneArrayStatsFuncPtr = af::array(*)(const af::array&, const dim_t);
using OneArrayStatsFunction = std::function<af::array(const af::array&, const dim_t)>;

static OneArrayStatsFunction getAfStdevFunction(bool isBiased)
{
#if AF_API_VERSION >= 38
    // af::stdev is overloaded, so we need this to narrow down to a single function.
    using AfStdevFuncPtr = af::array(*)(const af::array&, const af::varBias, const dim_t);

    return std::bind(
               static_cast<AfStdevFuncPtr>(af::stdev),
               std::placeholders::_1,
               getVarBias(isBiased),
               std::placeholders::_2);
#else
    if(isBiased)
    {
        throw Pothos::NotImplementedException("Biased stdev is only available with ArrayFire 3.8+.")
    }

    // af::stdev is overloaded, so we need this to narrow down to a single function.
    return static_cast<OneArrayStatsFuncPtr>(af::stdev);
#endif
}

static OneArrayStatsFunction getAfVarFunction(bool isBiased)
{
#if AF_API_VERSION >= 38
    // af::var is overloaded, so we need this to narrow down to a single function.
    using AfVarFuncPtr = af::array(*)(const af::array&, const af::varBias, const dim_t);

    return std::bind(
               static_cast<AfVarFuncPtr>(af::var),
               std::placeholders::_1,
               getVarBias(isBiased),
               std::placeholders::_2);
#else
    // af::var is overloaded, so we need this to narrow down to a single function.
    using AfVarFuncPtr = af::array(*)(const af::array&, const bool, const dim_t);

    return std::bind(
               static_cast<AfVarFuncPtr>(af::var),
               std::placeholders::_1,
               isBiased,
               std::placeholders::_2);
#endif
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

//
// Class
//

class OneArrayStatsBlock: public ArrayFireBlock
{
    public:

        static Pothos::Block* make(
            const std::string& device,
            const OneArrayStatsFunction& func,
            const DTypeSupport& dtypeSupport,
            const Pothos::DType& dtype)
        {
            validateDType(dtype, dtypeSupport);

            return new OneArrayStatsBlock(
                           device,
                           func,
                           dtype);
        }

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
            _afDType(Pothos::Object(dtype).convert<af::dtype>()),
            _lastValue(0.0)
        {
            this->setupInput(0, _dtype, _domain);
            this->setupOutput(0, _dtype, _domain);

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
            auto afLabelValues = _func(afArray.as(::f64), defaultDim);
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

class StdevBlock: public OneArrayStatsBlock
{
    public:

        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype)
        {
            return new StdevBlock(device, dtype);
        }

        StdevBlock(
            const std::string& device,
            const Pothos::DType& dtype
        ):
            OneArrayStatsBlock(
                device,
                getAfStdevFunction(false),
                dtype),
            _isBiased(false)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(StdevBlock, isBiased));
            this->registerCall(this, POTHOS_FCN_TUPLE(StdevBlock, setIsBiased));

            this->registerProbe("isBiased");
            this->registerSignal("isBiasedChanged");
        }

        bool isBiased() const
        {
            return _isBiased;
        };

        void setIsBiased(bool isBiased)
        {
            _isBiased = isBiased;
            _func = getAfStdevFunction(isBiased);

            this->emitSignal("isBiasedChanged", isBiased);
        }

    private:

        bool _isBiased;
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
                getAfVarFunction(isBiased),
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
            _func = getAfVarFunction(isBiased);

            this->emitSignal("isBiasedChanged", isBiased);
        }

    private:

        bool _isBiased;
};

//
// Factories
//

/*
 * |PothosDoc Mean (GPU)
 *
 * Calls <b>af::mean</b> on each input buffer to calculate the
 * arithmetic mean of the given values. The result of the last calculation
 * can be queried with the <b>lastValue</b> probe.
 *
 * |category /GPU/Statistics
 * |keywords statistics stats mean average
 * |factory /gpu/statistics/mean(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerMean(
    "/gpu/statistics/mean",
    Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
        .bind<OneArrayStatsFuncPtr>(&af::mean, 1)
        .bind<DTypeSupport>(DTypeSupport({true,true,true,false}), 2));

/*
 * |PothosDoc Median (GPU)
 *
 * Calls <b>af::median</b> on each input buffer to calculate the
 * median of the given values. The result of the last calculation
 * can be queried with the <b>lastValue</b> probe.
 *
 * |category /GPU/Statistics
 * |keywords statistics stats
 * |factory /gpu/statistics/median(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerMedian(
    "/gpu/statistics/median",
    Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
        .bind<OneArrayStatsFuncPtr>(&af::median, 1)
        .bind<DTypeSupport>({true,true,true,false}, 2));

/*
 * |PothosDoc RMS (GPU)
 *
 * Calculates the root mean square (RMS) of the given values.
 * The result of the last calculation can be queried with the
 * <b>lastValue</b> probe.
 *
 * |category /GPU/Statistics
 * |keywords statistics stats root mean square
 * |factory /gpu/statistics/rms(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerRMS(
    "/gpu/statistics/rms",
    Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
        .bind<OneArrayStatsFuncPtr>(&afRMS, 1)
        .bind<DTypeSupport>(DTypeSupport(floatOnlyDTypeSupport), 2));

/*
 * |PothosDoc Variance (GPU)
 *
 * Calls <b>af::var</b> on each input buffer to calculate the
 * median of the given values. The result of the last calculation
 * can be queried with the <b>lastValue</b> probe.
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
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
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
 * |PothosDoc Standard Deviation (GPU)
 *
 * Calls <b>af::stdev</b> on each input buffer to calculate the
 * standard deviation of the given values. The result of the last calculation
 * can be queried with the <b>lastValue</b> probe.
 *
 * |category /GPU/Statistics
 * |keywords statistics stats stddev
 * |factory /gpu/statistics/stdev(device,dtype)
 * |setter setIsBiased(isBiased)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param isBiased[Is Biased?] Whether or not the input values contain sample bias.
 * Only available with ArrayFire 3.8+.
 * |widget ToggleSwitch(on="True", off="False")
 * |default False
 */
static Pothos::BlockRegistry registerStdev(
    "/gpu/statistics/stdev",
    Pothos::Callable(&StdevBlock::make));

/*
 * |PothosDoc Median Absolute Deviation (GPU)
 *
 * Calculates the median absolute deviation of the given values.
 * The result of the last calculation can be queried with the
 * <b>lastValue</b> probe.
 *
 * |category /GPU/Statistics
 * |keywords statistics stats mad
 * |factory /gpu/statistics/medabsdev(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerMedAbsDev(
    "/gpu/statistics/medabsdev",
    Pothos::Callable(&OneArrayStatsBlock::makeFromFuncPtr)
        .bind<OneArrayStatsFuncPtr>(&afMedAbsDev, 1)
        .bind<DTypeSupport>(DTypeSupport({true,true,true,false}), 2));

}
