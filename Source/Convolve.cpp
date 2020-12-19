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

        ConvolveBaseBlock(
            const std::string& device,
            size_t dtypeDim,
            const Pothos::Callable& callable
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(callable),
                Pothos::DType(typeid(T), dtypeDim),
                Pothos::DType(typeid(T), dtypeDim)),
            _taps({T(1.0)}),
            _convMode(::AF_CONV_DEFAULT),
            _waitTaps(false),
            _waitTapsArmed(false)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, taps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, mode));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setMode));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, waitTaps));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setWaitTaps));

            this->registerProbe("taps");
            this->registerProbe("mode");

            this->registerSignal("modeChanged");

            // Emit the initial signals.
            this->setTaps(_taps);
            this->setMode(_convMode);
        }

        virtual ~ConvolveBaseBlock() = default;

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
            _func.bind(Pothos::Object(_taps).convert<af::array>(), 1);
            _waitTapsArmed = false; // We have taps
        }

        std::string mode() const
        {
            return Pothos::Object(_convMode).convert<std::string>();
        }

        void setMode(af::convMode convMode)
        {
            _convMode = convMode;
            _func.bind(_convMode, 2);

            this->emitSignal("modeChanged", _convMode);
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
        af::convMode _convMode;
        bool _waitTaps;
        bool _waitTapsArmed;
};

template <typename T>
class ConvolveBlock: public ConvolveBaseBlock<T>
{
    public:
        using Class = ConvolveBlock<T>;

        ConvolveBlock(const std::string& device, size_t dtypeDim):
            ConvolveBaseBlock<T>(
                device,
                dtypeDim,
                Pothos::Callable(&af::convolve1)),
            _convDomain(::AF_CONV_AUTO)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, domain));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setDomain));

            this->registerProbe("domain");
            this->registerSignal("domainChanged");

            // Emit the initial signal.
            this->setDomain(_convDomain);
        }

        virtual ~ConvolveBlock() = default;

        std::string domain() const
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
            return new ConvolveBlock<T>(device, dtype.dimension());

    ifTypeDeclareFactory(short)
    ifTypeDeclareFactory(int)
    ifTypeDeclareFactory(long long)
    ifTypeDeclareFactory(unsigned char)
    ifTypeDeclareFactory(unsigned short)
    ifTypeDeclareFactory(unsigned)
    ifTypeDeclareFactory(unsigned long long)
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
            return new FFTConvolveBlock<T>(device, dtype.dimension(), callableFFTConvolve);

    ifTypeDeclareFactory(short)
    ifTypeDeclareFactory(int)
    ifTypeDeclareFactory(long long)
    ifTypeDeclareFactory(unsigned char)
    ifTypeDeclareFactory(unsigned short)
    ifTypeDeclareFactory(unsigned)
    ifTypeDeclareFactory(unsigned long long)
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
 * |PothosDoc Convolve (GPU)
 *
 * Uses <b>af::convolve1</b> to convolve the input stream with user-provided filter
 * taps. The taps can be set at runtime by connecting the output of a FIR Designer
 * block to <b>"setTaps"</b>.
 *
 * |category /GPU/Signal
 * |keywords array tap taps convolution
 * |factory /gpu/signal/convolve(device,dtype)
 * |setter setTaps(taps)
 * |setter setMode(mode)
 * |setter setDomain(domain)
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
 * |param taps[Taps] The filter taps used in convolution.
 * |widget LineEdit()
 * |default [1.0]
 * |preview enable
 *
 * |param mode[Convolution Mode] Options:
 * <ul>
 * <li><b>"Default"</b>: don't expand convolution output</li>
 * <li><b>"Expand"</b>: expand convolution output to match input size</li>
 * </ul>
 * |widget ComboBox(editable=false)
 * |option [Default] "Default"
 * |option [Expand] "Expand"
 * |default "Default"
 * |preview enable
 *
 * |param domain[Domain] The domain in which to convolve (frequency vs. spatial). Options:
 * <ul>
 * <li><b>Auto:</b> Let ArrayFire choose domain based on input</li>
 * <li><b>Spatial:</b> Convolve signals in the spatial domain.</li>
 * <li><b>Frequency:</b> Convolve signals in the frequency domain.</li>
 * </ul>
 * |widget ComboBox(editable=false)
 * |option [Auto] "Auto"
 * |option [Spatial] "Spatial"
 * |option [Frequency] "Freq"
 * |default "Auto"
 * |preview enable
 *
 * |param waitTaps[Wait Taps] Wait for the taps to be set before allowing operation.
 * Use this mode when taps are set exclusively at runtime by the setTaps() slot.
 * |widget ToggleSwitch(on="True", off="False")
 * |default false
 * |preview disable
 */
static Pothos::BlockRegistry registerConvolve(
    "/gpu/signal/convolve",
    Pothos::Callable(&makeConvolve));

/*
 * |PothosDoc FFT Convolve (GPU)
 *
 * Uses <b>af::fftConvolve1</b> to convolve the input stream with user-provided filter
 * taps using an FFT. The taps can be set at runtime by connecting the output of a FIR Designer
 * block to <b>"setTaps"</b>.
 *
 * |category /GPU/Signal
 * |keywords array tap taps convolution
 * |factory /gpu/signal/fftconvolve(device,dtype)
 * |setter setTaps(taps)
 * |setter setMode(mode)
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
 * |param taps[Taps] The filter taps used in convolution.
 * |widget LineEdit()
 * |default [1.0]
 * |preview enable
 *
 * |param mode[Convolution Mode] Options:
 * <ul>
 * <li><b>"Default"</b>: don't expand convolution output</li>
 * <li><b>"Expand"</b>: expand convolution output to match input size</li>
 * </ul>
 * |widget ComboBox(editable=false)
 * |option [Default] "Default"
 * |option [Expand] "Expand"
 * |default "Default"
 * |preview enable
 *
 * |param waitTaps[Wait Taps] Wait for the taps to be set before allowing operation.
 * Use this mode when taps are set exclusively at runtime by the setTaps() slot.
 * |widget ToggleSwitch(on="True", off="False")
 * |default false
 * |preview disable
 */
static Pothos::BlockRegistry registerFFTConvolve(
    "/gpu/signal/fftconvolve",
    Pothos::Callable(&makeFFTConvolve));
