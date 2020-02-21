// Copyright (c) 2019-2020 Nicholas Corgan
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

        FIRBlock(
            const std::string& device,
            size_t dtypeDims
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(&af::fir),
                Pothos::DType::fromDType(Class::dtype, dtypeDims),
                Pothos::DType::fromDType(Class::dtype, dtypeDims)),
            _taps({T(1.0)}),
            _waitTaps(false),
            _waitTapsArmed(false)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setTaps));

            this->registerProbe("getTaps");
            this->registerSignal("tapsChanged");
        }

        virtual ~FIRBlock() = default;

        void activate() override
        {
            ArrayFireBlock::activate();

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

            _taps = std::move(taps);
            _func.bind(Pothos::Object(_taps).convert<af::array>(), 0);
            _waitTapsArmed = false; // We have taps

            this->emitSignal("tapsChanged", _taps);
        }

        bool getWaitTaps() const
        {
            return _waitTaps;
        }

        // TODO: initializer
        void setWaitTaps(bool waitTaps)
        {
            _waitTaps = waitTaps;
        }

        void work() override
        {
            // If specified, don't do anything until taps are explicitly set.
            if(_waitTapsArmed) return;

            OneToOneBlock::work();
        }

    private:
        std::vector<TapType> _taps;
        bool _waitTaps;
        bool _waitTapsArmed;
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

        IIRBlock(
            const std::string& device,
            size_t dtypeDims
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(&af::iir),
                Pothos::DType::fromDType(Class::dtype, dtypeDims),
                Pothos::DType::fromDType(Class::dtype, dtypeDims)),
            _feedForwardCoeffs({0.0676, 0.135, 0.0676}),
            _feedbackCoeffs({1, -1.142, 0.412}),
            _waitTaps(false),
            _waitTapsArmed(false)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getFeedForwardCoeffs));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setFeedForwardCoeffs));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getFeedbackCoeffs));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setFeedbackCoeffs));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setTaps));

            this->registerProbe("getTaps");
            this->registerSignal("tapsChanged");
        }

        virtual ~IIRBlock() = default;

        void activate() override
        {
            ArrayFireBlock::activate();

            _waitTapsArmed = _waitTaps;
        }

        std::vector<TapType> getFeedForwardCoeffs() const
        {
            return _feedForwardCoeffs;
        }

        void setFeedForwardCoeffs(const std::vector<TapType>& feedForwardCoeffs)
        {
            if(feedForwardCoeffs.empty())
            {
                throw Pothos::InvalidArgumentException("Coefficients cannot be empty.");
            }
            else if(feedForwardCoeffs.size() != _feedbackCoeffs.size())
            {
                throw Pothos::InvalidArgumentException(
                          "Feed-forward and feedback coefficients "
                          "must be the same size.");
            }

            _feedForwardCoeffs = feedForwardCoeffs;
            _func.bind(Pothos::Object(_feedForwardCoeffs).convert<af::array>(), 0);
            _waitTapsArmed = false; // We have taps

            this->emitAllCoeffsChanged();
        }

        std::vector<TapType> getFeedbackCoeffs() const
        {
            return _feedbackCoeffs;
        }

        void setFeedbackCoeffs(const std::vector<TapType>& feedbackCoeffs)
        {
            if(feedbackCoeffs.empty())
            {
                throw Pothos::InvalidArgumentException("Coefficients cannot be empty.");
            }
            else if(feedbackCoeffs.size() != _feedForwardCoeffs.size())
            {
                throw Pothos::InvalidArgumentException(
                          "Feed-forward and feedback coefficients "
                          "must be the same size.");
            }

            _feedbackCoeffs = feedbackCoeffs;
            _func.bind(Pothos::Object(_feedbackCoeffs).convert<af::array>(), 1);
            _waitTapsArmed = false; // We have taps

            this->emitAllCoeffsChanged();
        }

        std::vector<TapType> getTaps() const
        {
            std::vector<TapType> taps = _feedForwardCoeffs;
            taps.insert(taps.end(), _feedbackCoeffs.begin(), _feedbackCoeffs.end());

            return taps;
        }

        void setTaps(const std::vector<TapType>& taps)
        {
            if(taps.empty())
            {
                throw Pothos::InvalidArgumentException("Taps cannot be empty.");
            }
            else if(1 == (taps.size() % 2))
            {
                throw Pothos::InvalidArgumentException(
                          "When passing in both sets of coefficients, "
                          "the input must be of an even size.");
            }

            _feedForwardCoeffs = _feedbackCoeffs = taps;
            _feedForwardCoeffs.erase(
                _feedForwardCoeffs.begin() + (_feedForwardCoeffs.size()/2),
                _feedForwardCoeffs.end());
            _feedbackCoeffs.erase(
                _feedbackCoeffs.begin(),
                _feedbackCoeffs.begin() + (_feedbackCoeffs.size()/2));

            _func.bind(Pothos::Object(_feedForwardCoeffs).convert<af::array>(), 0);
            _func.bind(Pothos::Object(_feedbackCoeffs).convert<af::array>(), 1);
            _waitTapsArmed = false; // We have taps

            this->emitAllCoeffsChanged();
        }

        bool getWaitTaps() const
        {
            return _waitTaps;
        }

        void setWaitTaps(bool waitTaps)
        {
            _waitTaps = waitTaps;
        }

        void work() override
        {
            // If specified, don't do anything until taps are explicitly set.
            if(_waitTapsArmed) return;

            OneToOneBlock::work();
        }

    private:
        std::vector<TapType> _feedForwardCoeffs;
        std::vector<TapType> _feedbackCoeffs;
        bool _waitTaps;
        bool _waitTapsArmed;

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
    const std::string& device,
    const Pothos::DType& dtype)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new FIRBlock<T>(device,dtype.dimension());

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

static Pothos::Block* makeIIR(
    const std::string& device,
    const Pothos::DType& dtype)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new IIRBlock<T>(device,dtype.dimension());

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

static Pothos::BlockRegistry registerFIR(
    "/arrayfire/signal/fir",
    Pothos::Callable(&makeFIR));
static Pothos::BlockRegistry registerIIR(
    "/arrayfire/signal/iir",
    Pothos::Callable(&makeIIR));
