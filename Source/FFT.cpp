// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Logger.h>

#include <arrayfire.h>

#include <cmath>
#include <string>
#include <typeinfo>

//
// Misc
//

static bool isPowerOfTwo(size_t num)
{
    if(0 == num)
    {
        return false;
    }

    return (std::ceil(std::log2(num)) == std::floor(std::log2(num)));
}

static const std::string fftBlockPath = "/arrayfire/signal/fft";

//
// Block classes
//

using FFTInPlaceFunc = void(*)(af::array&, const double);

template <typename T>
class FFTInPlaceBlock: public ArrayFireBlock
{
    public:
        FFTInPlaceBlock(
            const FFTInPlaceFunc& func,
            size_t numBins,
            double norm,
            size_t numChans
        ):
            ArrayFireBlock(),
            _func(func),
            _numBins(numBins),
            _norm(0.0), // Set with class setter
            _nchans(numChans)
        {
            if(!isPowerOfTwo(numBins))
            {
                auto& logger = Poco::Logger::get(fftBlockPath);
                poco_warning(
                    logger,
                    "This block is most efficient when "
                    "numBins is a power of 2.");
            }

            static const Pothos::DType dtype(typeid(T));
            for(size_t chan = 0; chan < numChans; ++chan)
            {
                this->setupInput(chan, dtype);
                this->setupOutput(chan, dtype);
            }

            this->registerProbe(
                "getNormalizationFactor",
                "normalizationFactorChanged",
                "setNormalizationFactor");

            setNormalizationFactor(norm);
        }

        virtual ~FFTInPlaceBlock() = default;

        // Custom output buffer manager with slabs large enough for the FFT
        // result.
        Pothos::BufferManager::Sptr getOutputBufferManager(const std::string&, const std::string&) override
        {
            Pothos::BufferManagerArgs args;
            args.bufferSize = _numBins*sizeof(T);
            return Pothos::BufferManager::make("generic", args);
        }

        double getNormalizationFactor() const
        {
            return _norm;
        }

        void setNormalizationFactor(double norm)
        {
            _norm = norm;

            this->emitSignal("normalizationFatorChanged", _norm);
        }

        void work() override
        {
            auto elems = this->workInfo().minElements;
            if(0 == elems)
            {
                return;
            }

            auto afArray = this->getNumberedInputPortsAs2DAfArray();

            /*
             * Before ArrayFire 3.6, gfor was not thread-safe, as some
             * internal bookkeeping was stored globally. As of ArrayFire 3.6,
             * all of this stuff is thread-local, so we can take advantage of
             * it.
             */
            #if AF_CONFIG_PER_THREAD
            gfor(size_t chan, _nchans)
            #else
            for(size_t chan = 0; chan < _nchans; ++chan)
            #endif
            {
                af::array row(afArray.row(chan));
                _func(row, _norm);
                afArray(row) = row;
            }

            this->post2DAfArrayToNumberedOutputPorts(afArray);
        }

    private:
        FFTInPlaceFunc _func;
        size_t _numBins;
        double _norm;
        size_t _nchans;
};

//
// Factories
//

static Pothos::Block* makeFFT(
    const Pothos::DType& dtype,
    size_t numBins,
    double norm,
    size_t numChannels,
    bool inverse)
{
    FFTInPlaceFunc func = inverse ? &af::ifftInPlace : &af::fftInPlace;

    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new FFTInPlaceBlock<T>(func, numBins, norm, numChannels);

    ifTypeDeclareFactory(std::complex<float>)
    ifTypeDeclareFactory(std::complex<double>)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
}

//
// Block registries
//

static Pothos::BlockRegistry registerFFT(
    fftBlockPath,
    Pothos::Callable(&makeFFT));
