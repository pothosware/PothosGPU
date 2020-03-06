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

#if AF_API_VERSION_CURRENT >= 36

// TODO: set order
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
          _topKFunction(::AF_TOPK_MAX)
        {
            this->setupInput(0, dtype);
            this->setupOutput(0, dtype);

            this->registerProbe("lastValue");

            this->registerProbe("K");
            this->registerSignal("KChanged");
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

#endif
