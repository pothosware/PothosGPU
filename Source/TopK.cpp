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

class TopK: public ArrayFireBlock
{
    public:
        static Pothos::Block* make(
            const std::string& device,
            const std::string& dtype)
        {
            // All but complex
            static const DTypeSupport dtypeSupport{true,true,true,false};
            validateDType(dtype, dtypeSupport);

            return new TopK(device, dtype);
        }

        TopK(const std::string& device,
             const std::string& dtype)
        : ArrayFireBlock(device),
          _k(1),
          _topKFunction(::AF_TOPK_DEFAULT)
        {
            this->setupInput(0, dtype);
            this->setupOutput(0, dtype);

            this->registerProbe("lastValue");

            this->registerCall(this, POTHOS_FCN_TUPLE(TopK, K));
            this->registerCall(this, POTHOS_FCN_TUPLE(TopK, setK));
            this->registerCall(this, POTHOS_FCN_TUPLE(TopK, order));
            this->registerCall(this, POTHOS_FCN_TUPLE(TopK, setOrder));
            this->registerCall(this, POTHOS_FCN_TUPLE(TopK, lastValue));

            this->registerProbe("K");
            this->registerProbe("order");

            this->registerSignal("KChanged");
            this->registerSignal("orderChanged");
        }

        size_t K() const
        {
            return static_cast<size_t>(_k);
        }

        void setK(size_t k)
        {
            // For some reason, ArrayFire takes this in as an int, but it
            // has to be unsigned, so enforce that here.
            _k = static_cast<int>(k);

            this->emitSignal("KChanged", k);
        }

        std::string order() const
        {
            return Pothos::Object(_topKFunction).convert<std::string>();
        }

        void setOrder(af::topkFunction order)
        {
            _topKFunction = order;

            this->emitSignal(
                "orderChanged",
                Pothos::Object(_topKFunction).convert<std::string>());
        }

        Pothos::Object lastValue() const
        {
            return _lastValue;
        }

        void work() override
        {
            if(0 == this->workInfo().minElements)
            {
                return;
            }

            auto afArray = this->getInputPortAsAfArray(0);

            af::array vals, _;
            af::topk(vals, _, afArray, _k, -1, _topKFunction);

            // Store a vector of the correct type in a Pothos
            // object. Let callers deal with the extraction.
            _lastValue = afArrayToStdVector(vals);

            this->produceFromAfArray(0, afArray);
        }

    private:
        int _k;
        af::topkFunction _topKFunction;
        Pothos::Object _lastValue;
};

/*
 * |PothosDoc Top K
 *
 * |category /ArrayFire/Statistics
 * |keywords top min max k
 * |factory /arrayfire/statistics/topk(device,dtype)
 * |setter setK(K)
 * |setter setOrder(order)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param K[K] How many values to record.
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview enable
 *
 * |param order[Order] The element sorting order.
 * |widget ComboBox(editable=false)
 * |option [Default] "Default"
 * |option [Min] "Min"
 * |option [Max] "Max"
 * |default "Default"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerTopK(
    "/arrayfire/statistics/topk",
    Pothos::Callable(&TopK::make));
