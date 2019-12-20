// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <vector>

//
// Block classes
//

template <typename T>
class FIRBlock: public OneToOneBlock
{
    public:
        using Type = T;
        using Class = FIRBlock<T>;
        using TapType = typename Tap<T>::Type;

        static const Pothos::DType dtype;

        FIRBlock(size_t nchans):
            OneToOneBlock(
                Pothos::Callable(&af::fir),
                Class::dtype,
                Class::dtype,
                nchans),
            _taps()
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setTaps));

            this->registerProbe("getTaps", "tapsChanged", "setTaps");
        }

        virtual ~FIRBlock() = default;

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
            _func.bind(Pothos::Object(_taps).convert<af::array>(), 0);

            this->emitSignal("tapsChanged", _taps);
        }

    private:
        std::vector<TapType> _taps;
};

template <typename T>
const Pothos::DType FIRBlock<T>::dtype(typeid(T));

template <typename T>
class IIRBlock: public OneToOneBlock
{
    public:
        using Type = T;
        using Class = IIRBlock<T>;
        using TapType = typename Tap<T>::Type;

        static const Pothos::DType dtype;

        IIRBlock(size_t nchans):
            OneToOneBlock(
                Pothos::Callable(&af::iir),
                Class::dtype,
                Class::dtype,
                nchans),
            _feedForwardCoeffs({0.0676, 0.135, 0.0676}),
            _feedbackCoeffs({1, -1.142, 0.412})
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getFeedForwardCoeffs));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setFeedForwardCoeffs));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getFeedbackCoeffs));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setFeedbackCoeffs));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setTaps));

            this->registerProbe("getTaps", "tapsChanged", "setTaps");
        }

        virtual ~IIRBlock() = default;

        std::vector<TapType> getFeedForwardCoeffs() const
        {
            return _feedForwardCoeffs;
        }

        void setFeedForwardCoeffs(const Pothos::Object& feedForwardCoeffs)
        {
            // Explicitly convert to a vector of the correct type to prevent
            // implicit conversions from passing an incompatible type into
            // ArrayFire.
            auto ffCoeffs = Pothos::Object(feedForwardCoeffs).convert<std::vector<TapType>>();
            if(ffCoeffs.empty())
            {
                throw Pothos::InvalidArgumentException("Coefficients cannot be empty.");
            }
            else if(ffCoeffs.size() != _feedbackCoeffs.size())
            {
                throw Pothos::InvalidArgumentException(
                          "Feed-forward and feedback coefficients "
                          "must be the same size.");
            }

            _func.bind(Pothos::Object(_feedForwardCoeffs).convert<af::array>(), 0);
            this->emitAllCoeffsChanged();
        }

        std::vector<TapType> getFeedbackCoeffs() const
        {
            return _feedbackCoeffs;
        }

        void setFeedbackCoeffs(const Pothos::Object& feedbackCoeffs)
        {
            // Explicitly convert to a vector of the correct type to prevent
            // implicit conversions from passing an incompatible type into
            // ArrayFire.
            auto fbCoeffs = Pothos::Object(feedbackCoeffs).convert<std::vector<TapType>>();
            if(fbCoeffs.empty())
            {
                throw Pothos::InvalidArgumentException("Coefficients cannot be empty.");
            }
            else if(fbCoeffs.size() != _feedForwardCoeffs.size())
            {
                throw Pothos::InvalidArgumentException(
                          "Feed-forward and feedback coefficients "
                          "must be the same size.");
            }

            _func.bind(Pothos::Object(_feedbackCoeffs).convert<af::array>(), 1);
            this->emitAllCoeffsChanged();
        }

        std::vector<TapType> getTaps() const
        {
            std::vector<TapType> taps = _feedForwardCoeffs;
            taps.insert(taps.end(), _feedbackCoeffs.begin(), _feedbackCoeffs.end());

            return taps;
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
            else if(1 == (__taps.size() % 2))
            {
                throw Pothos::InvalidArgumentException(
                          "When passing in both sets of coefficients, "
                          "the input must be of an even size.");
            }

            _feedForwardCoeffs = _feedbackCoeffs = __taps;
            _feedForwardCoeffs.erase(
                _feedForwardCoeffs.begin() + (_feedForwardCoeffs.size()/2),
                _feedForwardCoeffs.end());
            _feedbackCoeffs.erase(
                _feedbackCoeffs.begin(),
                _feedbackCoeffs.begin() + (_feedbackCoeffs.size()/2));

            _func.bind(Pothos::Object(_feedForwardCoeffs).convert<af::array>(), 0);
            _func.bind(Pothos::Object(_feedbackCoeffs).convert<af::array>(), 1);

            this->emitSignal("feedForwardCoeffsChanged", _feedForwardCoeffs);
            this->emitSignal("feedbackCoeffsChanged", _feedbackCoeffs);
            this->emitSignal("tapsChanged", __taps);
        }

    private:
        std::vector<TapType> _feedForwardCoeffs;
        std::vector<TapType> _feedbackCoeffs;

        void emitAllCoeffsChanged()
        {
            this->emitSignal("feedForwardCoeffsChanged", _feedForwardCoeffs);
            this->emitSignal("feedbackCoeffsChanged", _feedbackCoeffs);

            std::vector<TapType> taps = _feedForwardCoeffs;
            taps.insert(taps.end(), _feedbackCoeffs.begin(), _feedbackCoeffs.end());
            this->emitSignal("tapsChanged", taps);
        }
};

template <typename T>
const Pothos::DType IIRBlock<T>::dtype(typeid(T));

//
// Factories
//

static Pothos::Block* makeFIR(
    const Pothos::DType& dtype,
    size_t nchans)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new FIRBlock<T>(nchans);

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

static Pothos::Block* makeIIR(
    const Pothos::DType& dtype,
    size_t nchans)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new IIRBlock<T>(nchans);

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

static Pothos::BlockRegistry registerFIR(
    "/arrayfire/signal/fir",
    Pothos::Callable(&makeFIR));
static Pothos::BlockRegistry registerIIR(
    "/arrayfire/signal/iir",
    Pothos::Callable(&makeIIR));
