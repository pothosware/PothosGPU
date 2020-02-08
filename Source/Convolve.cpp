// Copyright (c) 2019-2020 Nicholas Corgan
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
// Block classes
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
            const std::string& device,
            const Pothos::Callable& callable
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(callable),
                Class::dtype,
                Class::dtype),
            _taps({T(1.0)}),
            _convMode(::AF_CONV_DEFAULT),
            _waitTaps(false),
            _waitTapsArmed(false)
        {
            // Emit the initial signals.
            this->setTaps(_taps);
            this->setMode(_convMode);

            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getMode));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setMode));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getWaitTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setWaitTaps));

            this->registerProbe("getTaps", "tapsChanged", "setTaps");
            this->registerProbe("getMode", "modeChanged", "setMode");
            this->registerProbe("getWaitTaps", "waitTapsChanged", "setWaitTaps");
        }

        virtual ~ConvolveBaseBlock() = default;

        void activate() override
        {
            _waitTapsArmed = _waitTaps;
        }

        std::vector<TapType> getTaps() const
        {
            return _taps;
        }

        void setTaps(const std::vector<TapType>& taps)
        {
            if(taps.empty())
            {
                throw Pothos::InvalidArgumentException("Taps cannot be empty.");
            }

            _taps = taps;
            _func.bind(Pothos::Object(_taps).convert<af::array>(), 1);
            _waitTapsArmed = false; // We have taps

            this->emitSignal("tapsChanged", _taps);
        }

        std::string getMode() const
        {
            return Pothos::Object(_convMode).convert<std::string>();
        }

        void setMode(af::convMode convMode)
        {
            _convMode = convMode;
            _func.bind(_convMode, 2);

            this->emitSignal("modeChanged", _convMode);
        }

        bool getWaitTaps() const
        {
            return _waitTaps;
        }

        void setWaitTaps(bool waitTaps)
        {
            _waitTaps = waitTaps;

            this->emitSignal("waitTapsChanged", waitTaps);
        }

        void work() override
        {
            // If specified, don't do anything until taps are explicitly set.
            if(_waitTapsArmed) return;

            OneToOneBlock::work();
        }

    private:
        std::vector<TapType> _taps;
        af::convMode _convMode;
        bool _waitTaps;
        bool _waitTapsArmed;
};

template <typename T>
const Pothos::DType ConvolveBaseBlock<T>::dtype(typeid(T));

template <typename T>
class ConvolveBlock: public ConvolveBaseBlock<T>
{
    public:
        using Class = ConvolveBlock<T>;

        ConvolveBlock(const std::string& device):
            ConvolveBaseBlock<T>(
                device,
                Pothos::Callable(&af::convolve1)),
            _convDomain(::AF_CONV_AUTO)
        {
            // Emit the initial signal.
            this->setDomain(_convDomain);

            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getDomain));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setDomain));

            this->registerProbe("getDomain", "domainChanged", "setDomain");
        }

        virtual ~ConvolveBlock() = default;

        std::string getDomain() const
        {
            return Pothos::Object(_convDomain).convert<std::string>();
        }

        void setDomain(af::convDomain convDomain)
        {
            _convDomain = convDomain;
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
    const std::string& device,
    const Pothos::DType& dtype)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new ConvolveBlock<T>(device);

    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::int64_t)
    ifTypeDeclareFactory(std::uint8_t)
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    ifTypeDeclareFactory(std::uint64_t)
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
    const std::string& device,
    const Pothos::DType& dtype)
{
    static const Pothos::Callable callableFFTConvolve(&af::fftConvolve1);

    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new FFTConvolveBlock<T>(device, callableFFTConvolve);

    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::int64_t)
    ifTypeDeclareFactory(std::uint8_t)
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    ifTypeDeclareFactory(std::uint64_t)
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
