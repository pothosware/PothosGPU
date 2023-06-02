// Copyright (c) 2019-2021,2023 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

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
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, waitTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setWaitTaps));
        }

        virtual ~FIRBlock() = default;

        void activate() override
        {
            ArrayFireBlock::activate();

            _waitTapsArmed = _waitTaps;
        }

        std::vector<TapType> taps() const
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
            _func.bind(Pothos::Object(_taps).convert<af::array>(), 0);
            _waitTapsArmed = false; // We have taps
        }

        bool waitTaps() const
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

        // TODO: this is the case as of ArrayFire 3.7.0.
        //       #ifdef this if it changes.
        static constexpr size_t MaxFFCoeffLength = 512;

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
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, waitTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setWaitTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setFeedForwardCoeffs));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setFeedbackCoeffs));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setTapsFromCommsIIRDesigner));
        }

        virtual ~IIRBlock() = default;

        void activate() override
        {
            ArrayFireBlock::activate();

            _waitTapsArmed = _waitTaps;
        }

        void setFeedForwardCoeffs(const std::vector<TapType>& feedForwardCoeffs)
        {
            if(feedForwardCoeffs.empty())
            {
                throw Pothos::InvalidArgumentException("Coefficients cannot be empty.");
            }
            else if(feedForwardCoeffs.size() > MaxFFCoeffLength)
            {
                throw Pothos::InvalidArgumentException(
                          Poco::format(
                              "In ArrayFire %s, af::iir only accepts feed-forward "
                              "coefficients up to length %s",
                              std::string(AF_VERSION),
                              Poco::NumberFormatter::format(MaxFFCoeffLength)));
            }

            _feedForwardCoeffs = feedForwardCoeffs;
            _func.bind(Pothos::Object(_feedForwardCoeffs).convert<af::array>(), 0);
            _disarmWaitTapsIfCoeffsPopulated();
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
            _disarmWaitTapsIfCoeffsPopulated();
        }

        /*
         * /comms/iir_designer emits a single tap vector that contains both the
         * feed-forward and feedback taps in a flattened array. This is restricted
         * to the taps being the same length.
         */
        void setTapsFromCommsIIRDesigner(const std::vector<TapType>& taps)
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

            auto feedForwardCoeffs = taps;
            auto feedbackCoeffs = taps;
            feedForwardCoeffs.erase(
                feedForwardCoeffs.begin() + (feedForwardCoeffs.size()/2),
                feedForwardCoeffs.end());
            feedbackCoeffs.erase(
                feedbackCoeffs.begin(),
                feedbackCoeffs.begin() + (feedbackCoeffs.size()/2));

            this->setFeedForwardCoeffs(feedForwardCoeffs);
            this->setFeedbackCoeffs(feedbackCoeffs);
        }

        bool waitTaps() const
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

        inline void _disarmWaitTapsIfCoeffsPopulated()
        {
            _waitTapsArmed = _feedForwardCoeffs.empty() || _feedbackCoeffs.empty();
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

    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)
    ifTypeDeclareFactory(std::complex<float>)
    ifTypeDeclareFactory(std::complex<double>)
    #undef ifTypeDeclareFactory

    throw Pothos::InvalidArgumentException(
              "Unsupported type.",
              dtype.name());
}

//
// Block registries
//

/*
 * |PothosDoc FIR Filter (GPU)
 *
 * Uses <b>af::fir</b> to convolve the input stream with user-provided filter
 * taps. The taps can be set at runtime by connecting the output of a FIR Designer
 * block to <b>"setTaps"</b>.
 *
 * |category /GPU/Signal
 * |category /Filter
 * |keywords array tap taps fir
 * |factory /gpu/signal/fir_filter(device,dtype)
 * |setter setTaps(taps)
 * |setter setWaitTaps(waitTaps)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(float=1,cfloat=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param taps[Taps] The FIR filter taps used in convolution.
 * |widget LineEdit()
 * |default [1.0]
 * |preview enable
 *
 * |param waitTaps[Wait Taps] Wait for the taps to be set before allowing operation.
 * Use this mode when taps are set exclusively at runtime by the setTaps() slot.
 * |widget ToggleSwitch(on="True", off="False")
 * |default false
 * |preview disable
 */
static Pothos::BlockRegistry registerFIR(
    "/gpu/signal/fir_filter",
    Pothos::Callable(&makeFIR));


/*
 * |PothosDoc IIR Filter (GPU)
 *
 * Uses <b>af::iir</b> to convolve the input stream with user-provided filter
 * taps. The individual coefficient parts can be set at runtime by connecting
 * the outputs of a designer block to <b>"setFeedForwardCoeffs"</b> and
 * <b>"setFeedbackCoeffs"</b>. Alternatively, the output of <b>/comms/iir_designer</b>
 * can be connected to <b>setTapsFromCommsIIRDesigner</b> to set both sets of
 * coefficients simultaneously.
 *
 * |category /GPU/Signal
 * |category /Filter/GPU
 * |keywords array tap taps iir
 * |factory /gpu/signal/iir_filter(device,dtype)
 * |setter setFeedForwardCoeffs(feedForwardCoeffs)
 * |setter setFeedbackCoeffs(feedbackCoeffs)
 * |setter setTapsFromCommsIIRDesigner(taps)
 * |setter setWaitTaps(waitTaps)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(float=1,cfloat=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param feedForwardCoeffs[Feed-Forward Coefficients]
 * |widget LineEdit()
 * |default [0.0676, 0.135, 0.0676]
 * |preview enable
 *
 * |param feedbackCoeffs[Feedback Coefficients]
 * |widget LineEdit()
 * |default [1, -1.142, 0.412]
 * |preview enable
 *
 * |param taps[Taps] The combined feed-forward and feedback coefficients.
 * This parameter is only intended to be used with <b>/comms/iir_designer</b>.
 * |widget LineEdit()
 * |default [0.0676, 0.135, 0.0676, 1, -1.142, 0.412]
 * |preview disable
 *
 * |param waitTaps[Wait Taps] Wait for the taps to be set before allowing operation.
 * Use this mode when taps are set exclusively at runtime by the setTaps() slot.
 * |widget ToggleSwitch(on="True", off="False")
 * |default false
 * |preview disable
 */
static Pothos::BlockRegistry registerIIR(
    "/gpu/signal/iir_filter",
    Pothos::Callable(&makeIIR));
