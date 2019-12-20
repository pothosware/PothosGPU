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
// TODO: IIR
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
            const Pothos::Object& taps,
            size_t nchans
        ):
            OneToOneBlock(
                Pothos::Callable(&af::fir),
                Class::dtype,
                Class::dtype,
                nchans),
            _taps()
        {
            // Each of these calls validates its given parameter.
            this->setTaps(taps);

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

//
// Factories
//

static Pothos::Block* makeFIR(
    const Pothos::DType& dtype,
    const Pothos::Object& taps,
    size_t nchans)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new FIRBlock<T>(taps, nchans);

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
