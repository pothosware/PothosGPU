// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Timestamp.h>

#include <arrayfire.h>

#include <cassert>
#include <string>
#include <typeinfo>

#if AF_API_VERSION_CURRENT >= 34

using AfRandomFunc = af::array(*)(const af::dim4&, const af::dtype, af::randomEngine&);

class RandomBlock: public ArrayFireBlock
{
    public:

        using SeedType = unsigned long long;

        static Pothos::Block* make(
            const Pothos::DType& dtype,
            const std::string& distribution,
            size_t numOutputs)
        {
            return new RandomBlock(dtype, distribution, numOutputs);
        }

        RandomBlock(
            const Pothos::DType& dtype,
            const std::string& distribution,
            size_t numOutputs
        ):
            ArrayFireBlock(),
            _numOutputs(static_cast<dim_t>(numOutputs)),
            _afRandomFunc(nullptr), // Set in constructor
            _distribution(), // Set in constructor
            _afDType(Pothos::Object(dtype).convert<af::dtype>()),
            _afRandomEngine()
        {
            this->setDistribution(distribution);

            this->reseedRandomEngineWithTime();

            for(size_t chan = 0; chan < numOutputs; ++chan)
            {
                this->setupOutput(chan, dtype);
            }

            this->registerCall(this, POTHOS_FCN_TUPLE(RandomBlock, getDistribution));
            this->registerCall(this, POTHOS_FCN_TUPLE(RandomBlock, getDistribution));
            this->registerCall(this, POTHOS_FCN_TUPLE(RandomBlock, setRandomEngineType));
            this->registerCall(this, POTHOS_FCN_TUPLE(RandomBlock, setRandomEngineType));

            // This call is overloaded, so no macro for us.
            this->registerCall(this, "reseedRandomEngine", &RandomBlock::reseedRandomEngineWithTime);
            this->registerCall(this, "reseedRandomEngine", &RandomBlock::reseedRandomEngine);

            this->registerProbe(
                "getDistribution",
                "distributionChanged",
                "setDistribution");
            this->registerProbe(
                "getRandomEngineType",
                "randomEngineTypeChanged",
                "setRandomEngineType");
        }

        std::string getDistribution() const
        {
            return _distribution;
        }

        void setDistribution(const std::string& distribution)
        {
            if("UNIFORM" == distribution)
            {
                this->_afRandomFunc = &af::randu;
            }
            else if("NORMAL" == distribution)
            {
                this->_afRandomFunc = &af::randn;
            }
            else
            {
                throw Pothos::InvalidArgumentException(
                          "Invalid distribution",
                          distribution);
            }

            this->_distribution = distribution;
            this->emitSignal("distributionChanged", distribution);
        }

        std::string getRandomEngineType() const
        {
            return Pothos::Object(_afRandomEngine.getType()).convert<std::string>();
        }

        void setRandomEngineType(const std::string& randomEngineType)
        {
            _afRandomEngine.setType(Pothos::Object(randomEngineType).convert<af::randomEngineType>());
            this->emitSignal("randomEngineTypeChanged", randomEngineType);
        }

        void reseedRandomEngineWithTime()
        {
            // Seed this random engine with the current time.
            const auto seed = static_cast<SeedType>(Poco::Timestamp().epochMicroseconds());
            this->reseedRandomEngine(seed);
        }

        void reseedRandomEngine(SeedType seed)
        {
            _afRandomEngine.setSeed(seed);
        }

        void work() override
        {
            const af::dim4 dims(_numOutputs, BufferLen);

            auto afOutput = _afRandomFunc(dims, _afDType, _afRandomEngine);
            this->post2DAfArrayToNumberedOutputPorts(afOutput);
        }

    private:

        static constexpr dim_t BufferLen = 8192;

        dim_t _numOutputs;

        AfRandomFunc _afRandomFunc;
        std::string _distribution;
        af::dtype _afDType;
        mutable af::randomEngine _afRandomEngine;
};

static Pothos::BlockRegistry registerRandomSource(
    "/arrayfire/random/source",
    Pothos::Callable(&RandomBlock::make));

#endif
