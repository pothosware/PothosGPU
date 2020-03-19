// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Logger.h>

#include <arrayfire.h>

#include <cmath>
#include <functional>
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

using FFTFuncPtr = af::array(*)(const af::array&, const double);
using FFTFunc = std::function<af::array(const af::array&, const double)>;

template <typename In, typename Out>
class FFTBlock: public ArrayFireBlock
{
    public:
        using InType = In;
        using OutType = Out;
        using Class = FFTBlock<In, Out>;

        FFTBlock(
            const std::string& device,
            FFTFunc func,
            size_t numBins,
            double norm,
            size_t dtypeDims,
            bool checkNumBins
        ):
            ArrayFireBlock(device),
            _func(func),
            _numBins(numBins),
            _norm(0.0) // Set with class setter
        {
            if(checkNumBins && !isPowerOfTwo(numBins))
            {
                static auto& logger = Poco::Logger::get(fftBlockPath);
                poco_warning(
                    logger,
                    "This block is most efficient when "
                    "numBins is a power of 2.");
            }

            static const Pothos::DType inDType(typeid(InType));
            static const Pothos::DType outDType(typeid(OutType));

            this->setupInput(
                0,
                Pothos::DType::fromDType(inDType, dtypeDims));
            this->setupOutput(
                0,
                Pothos::DType::fromDType(outDType, dtypeDims));

            this->registerProbe("normalizationFactor");
            this->registerSignal("normalizationFactorChanged");

            this->setNormalizationFactor(norm);

            this->registerCall(this, POTHOS_FCN_TUPLE(Class, normalizationFactor));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setNormalizationFactor));
        }

        virtual ~FFTBlock() = default;

        double normalizationFactor() const
        {
            return _norm;
        }

        void setNormalizationFactor(double norm)
        {
            _norm = norm;

            this->emitSignal("normalizationFactorChanged", _norm);
        }

        void work() override
        {
            auto elems = this->workInfo().minElements;
            if(elems < this->_numBins)
            {
                return;
            }

            auto afInput = this->getInputPortAsAfArray(0);
            auto afOutput = _func(afInput, this->_norm);
            this->postAfArray(0, afOutput);
        }

    private:
        FFTFunc _func;

    protected:
        size_t _numBins;
        double _norm;
        size_t _nchans;
};

//
// Generate underlying FFT functions based on types
//

template <typename T, typename U, typename V>
using EnableIfComplexAndFloat = typename std::enable_if<IsComplex<T>::value && !IsComplex<U>::value, V>::type;

template <typename T, typename U, typename V>
using EnableIfFloatAndComplex = typename std::enable_if<!IsComplex<T>::value && IsComplex<U>::value, V>::type;

template <typename T, typename U, typename V>
using EnableIfBothComplex = typename std::enable_if<IsComplex<T>::value && IsComplex<U>::value, V>::type;

template <typename FwdIn, typename FwdOut>
static EnableIfBothComplex<FwdIn, FwdOut, FFTFunc> getFFTFunc(
    size_t /*numBins*/,
    bool inverse)
{
    // To despecialize
    using FuncPtr = af::array(*)(const af::array&, const double, const dim_t);

    FuncPtr func = inverse ? FuncPtr(&af::ifftNorm) : FuncPtr(&af::fftNorm);

    auto retLambda = [func](const af::array& arr, const double norm)
                     {
                         // Pass in 0 to not pad or truncate output array.
                         return func(arr, norm, 0);
                     };

    return FFTFunc(retLambda);
}

// This is here for compatibility, but there is no inverse case for this.
template <typename FwdIn, typename FwdOut>
static EnableIfFloatAndComplex<FwdIn, FwdOut, FFTFunc> getFFTFunc(
    size_t /*numBins*/,
    bool inverse)
{
    if(inverse)
    {
        static const Pothos::DType fwdInDType(typeid(FwdIn));
        static const Pothos::DType fwdOutDType(typeid(FwdOut));
        
        throw Pothos::InvalidArgumentException(
                  Poco::format(
                      "Reverse FFT is not supported for %s -> %s",
                      fwdInDType.name(),
                      fwdOutDType.name()));
    }

    // To despecialize
    using FuncPtr = af::array(*)(const af::array&, const double);

    return FuncPtr(af::fftR2C<1>);
}

template <typename FwdIn, typename FwdOut>
static EnableIfComplexAndFloat<FwdIn, FwdOut, FFTFunc> getFFTFunc(
    size_t numBins,
    bool inverse)
{
    const bool isOdd = (1 == (numBins % 2));

    auto fwdRetLambda = [isOdd](const af::array& arr, const double norm)
    {
        // Since we know numBins beforehand, remove the parameter.
        return af::fftC2R<1>(arr, isOdd, norm);
    };
    auto revRetLambda = [](const af::array& arr, const double norm)
    {
         // Pass in 0 to not pad or truncate output array.
        return af::ifftNorm(arr, norm, 0);
    };

    return inverse ? FFTFunc(revRetLambda) : FFTFunc(fwdRetLambda);
}

//
// Factories
//

static Pothos::Block* makeFFT(
    const std::string& device,
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType,
    size_t numBins,
    double norm,
    bool inverse)
{
    if(inputDType.dimension() != outputDType.dimension())
    {
        throw Pothos::InvalidArgumentException("Input and output type dimensions must match.");
    }

    #define __ifTypeDeclareFactory(FwdIn,FwdOut) \
    if((Pothos::DType::fromDType(inputDType, 1) == Pothos::DType(typeid(FwdIn))) && \
       (Pothos::DType::fromDType(outputDType, 1) == Pothos::DType(typeid(FwdOut)))) \
    { \
        auto fftFunc = getFFTFunc<FwdIn,FwdOut>(numBins, inverse); \
        if(inverse) return new FFTBlock<FwdOut,FwdIn>(device, fftFunc, numBins, norm, inputDType.dimension(), false); \
        else        return new FFTBlock<FwdIn,FwdOut>(device, fftFunc, numBins, norm, inputDType.dimension(), true); \
    }
    #define ifTypeDeclareFactory(FloatType) \
        __ifTypeDeclareFactory(FloatType, std::complex<FloatType>) \
        __ifTypeDeclareFactory(std::complex<FloatType>, FloatType) \
        __ifTypeDeclareFactory(std::complex<FloatType>, std::complex<FloatType>)

    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
              Poco::format(
                  "Unsupported types: %s -> %s",
                  inputDType.name(),
                  outputDType.name()));
}

//
// Block registration
//

/*
 * |PothosDoc FFT
 *
 * Calculates the FFT of the input stream, with an optional normalization factor.
 *
 * |category /ArrayFire/Signal
 * |keywords array signal fft ifft fourier
 * |factory /arrayfire/signal/fft(device,inputDType,outputDType,numBins,norm,inverse)
 * |setter setNormalizationFactor(norm)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param inputDType[Input Data Type] The forward FFT's input data type.
 * |widget DTypeChooser(float=1,cfloat=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param outputDType[Output Data Type] The forward FFT's output data type.
 * |widget DTypeChooser(float=1,cfloat=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param numBins[Num FFT Bins] The number of bins per FFT.
 * |default 1024
 * |option 512
 * |option 1024
 * |option 2048
 * |option 4096
 * |widget ComboBox(editable=true)
 * |preview enable
 *
 * |param norm[Normalization Factor] Multiply all outputs by this value post-transformation.
 * |widget DoubleSpinBox(minimum=0.0)
 * |default 1.0
 * |preview enable
 *
 * |param inverse[Inverse?]
 * |widget ToggleSwitch(on="True",off="False")
 * |preview enable
 * |default false
 */
static Pothos::BlockRegistry registerFFT(
    fftBlockPath,
    Pothos::Callable(&makeFFT));
