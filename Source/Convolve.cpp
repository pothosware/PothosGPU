// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <string>
#include <vector>

// Resolve overloads
using FFTConvolveFuncPtr = af::array(*)(
                               const af::array&,
                               const af::array&,
                               const af::convMode);

//
// Test classes
//

template <typename T>
class ConvolveBaseBlock: public OneToOneBlock
{
    public:
        using Type = T;
        using Class = ConvolveBaseBlock<T>;
        using TapType = typename Tap<T>::Type;

        static const Pothos::DType dtype;

        ConvolveBaseBlock(
            const Pothos::Callable& callable,
            const Pothos::Object& taps,
            const Pothos::Object& mode,
            size_t nchans
        ):
            OneToOneBlock(
                Pothos::Callable(callable),
                Class::dtype,
                Class::dtype,
                nchans),
            _taps(),
            _convMode(::AF_CONV_DEFAULT)
        {
            // Each of these calls validates its given parameter.
            this->setTaps(taps);
            this->setMode(mode);

            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getMode));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setMode));

            this->registerProbe("getTaps", "tapsChanged", "setTaps");
            this->registerProbe("getMode", "modeChanged", "setMode");
        }

        virtual ~ConvolveBaseBlock() = default;

        std::vector<TapType> getTaps() const
        {
            return _taps;
        }

        void setTaps(const Pothos::Object& taps)
        {
            // Explicitly convert to a vector of the correct type to prevent
            // implicit conversions from passing an incompatible type into
            // ArrayFire.
            auto __taps = Pothos::Object(taps).convert<std::vector<TapType>>();
            if(__taps.empty())
            {
                throw Pothos::InvalidArgumentException("Taps cannot be empty.");
            }

            _taps = std::move(__taps);
            _func.bind(Pothos::Object(_taps).convert<af::array>(), 1);

            this->emitSignal("tapsChanged", _taps);
        }

        std::string getMode() const
        {
            return Pothos::Object(_convMode).convert<std::string>();
        }

        void setMode(const Pothos::Object& mode)
        {
            _convMode = mode.convert<af::convMode>();
            _func.bind(_convMode, 2);

            this->emitSignal("modeChanged", _convMode);
        }

    private:
        std::vector<TapType> _taps;
        af::convMode _convMode;
};

template <typename T>
const Pothos::DType ConvolveBaseBlock<T>::dtype(typeid(T));

template <typename T>
class ConvolveBlock: public ConvolveBaseBlock<T>
{
    public:
        using Class = ConvolveBlock<T>;

        ConvolveBlock(
            const Pothos::Object& taps,
            const Pothos::Object& mode,
            const Pothos::Object& domain,
            size_t nchans
        ):
            ConvolveBaseBlock<T>(
                Pothos::Callable(&af::convolve1),
                taps,
                mode,
                nchans),
            _convDomain(::AF_CONV_AUTO)
        {
            // This will validate the parameter.
            this->setDomain(domain);

            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getMode));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setMode));

            this->registerProbe("getDomain", "domainChanged", "setDomain");
        }

        virtual ~ConvolveBlock() = default;

        std::string getDomain() const
        {
            return Pothos::Object(_convDomain).convert<std::string>();
        }

        void setDomain(const Pothos::Object& domain)
        {
            _convDomain = domain.convert<af::convDomain>();
            this->_func.bind(_convDomain, 3);

            this->emitSignal("domainChanged", _convDomain);
        }

    private:
        af::convDomain _convDomain;
};

template <typename T>
using FFTConvolveBlock = ConvolveBaseBlock<T>;

//
// Factories
//

static Pothos::Block* makeConvolve(
    const Pothos::DType& dtype,
    const Pothos::Object& taps,
    const Pothos::Object& mode,
    const Pothos::Object& domain,
    size_t nchans)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new ConvolveBlock<T>(taps, mode, domain, nchans);

    // TODO: 64-bit int types
    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::uint8_t)
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)
    ifTypeDeclareFactory(std::complex<float>)
    ifTypeDeclareFactory(std::complex<double>)
    #undef ifTypeDeclareFactory

    throw Pothos::InvalidArgumentException(
              "Unsupported type.",
              dtype.name());
}

static Pothos::Block* makeFFTConvolve(
    const Pothos::DType& dtype,
    const Pothos::Object& taps,
    const Pothos::Object& mode,
    size_t nchans)
{
    static const Pothos::Callable callableFFTConvolve(&af::fftConvolve1);

    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new FFTConvolveBlock<T>(callableFFTConvolve, taps, mode, nchans);

    // TODO: 64-bit int types
    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::uint8_t)
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)
    ifTypeDeclareFactory(std::complex<float>)
    ifTypeDeclareFactory(std::complex<double>)
    #undef ifTypeDeclareFactory

    throw Pothos::InvalidArgumentException(
              "Unsupported type.",
              dtype.name());
}

static Pothos::BlockRegistry registerConvolve(
    "/arrayfire/signal/convolve",
    Pothos::Callable(&makeConvolve));
static Pothos::BlockRegistry registerFFTConvolve(
    "/arrayfire/signal/fftconvolve",
    Pothos::Callable(&makeFFTConvolve));
