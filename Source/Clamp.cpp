// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>

#include <arrayfire.h>

#include <cstdint>
#include <iostream>
#include <typeinfo>

#if AF_API_VERSION_CURRENT >= 34

template <typename T>
class Clamp: public OneToOneBlock
{
    public:
        using Class = Clamp<T>;
        using AFType = typename PothosToAF<T>::type;

        static const Pothos::DType dtype;

        Clamp(
            const T& minValue,
            const T& maxValue,
            size_t nchans
        ):
            OneToOneBlock(
                Pothos::Callable(),
                dtype,
                dtype,
                nchans)
        {
            validateMinMax(minValue, maxValue);

            _afMinValue = PothosToAF<T>::to(minValue);
            _afMaxValue = PothosToAF<T>::to(maxValue);

            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getMinValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setMinValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getMaxValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setMaxValue));

            this->registerProbe("getMinValue", "minValueChanged", "setMinValue");
            this->registerProbe("getMaxValue", "maxValueChanged", "setMaxValue");
        }

        virtual ~Clamp() = default;

        T getMinValue() const
        {
            return PothosToAF<T>::from(_afMinValue);
        }

        void setMinValue(const T& minValue)
        {
            validateMinMax(minValue, getMaxValue());

            _afMinValue = PothosToAF<T>::to(minValue);
            this->emitSignal("minValueChanged", minValue);
        }

        T getMaxValue() const
        {
            return PothosToAF<T>::from(_afMaxValue);
        }

        void setMaxValue(const T& maxValue)
        {
            validateMinMax(getMinValue(), maxValue);

            _afMaxValue = PothosToAF<T>::to(maxValue);
            this->emitSignal("maxValueChanged", maxValue);
        }

        void work(const af::array& afInput) override;

    private:
        AFType _afMinValue;
        AFType _afMaxValue;

        void validateMinMax(const T& min, const T& max)
        {
            if(min > max)
            {
                throw Pothos::InvalidArgumentException(
                          "minValue must be < maxValue",
                          Poco::format("{0} > {1}", min, max));
            }
        }
};

template <typename T>
const Pothos::DType Clamp<T>::dtype = Pothos::DType(typeid(T));

/*
 * We have two implementations here: a general one, and an optimized one for
 * doubles. For some reason, ArrayFire's clamp function only has a double
 * overload for scalar boundaries, despite supporting all types.
 */

template <typename T>
void Clamp<T>::work(const af::array& afInput)
{
    static const af::dtype afDType = Pothos::Object(dtype).convert<af::dtype>();

    const size_t elems = this->workInfo().minElements;
    assert(0 < elems);

    af::array afArrayMinValue;
    af::array afArrayMaxValue;

    if(1 == _nchans)
    {
        afArrayMinValue = af::constant(_afMinValue, elems, afDType);
        afArrayMaxValue = af::constant(_afMaxValue, elems, afDType);
        auto afOutput = af::clamp(afInput, afArrayMinValue, afArrayMaxValue);

        this->input(0)->consume(elems);
        this->output(0)->postBuffer(Pothos::Object(afOutput)
                                        .convert<Pothos::BufferChunk>());
    }
    else
    {
        assert(0 != _nchans);
        assert(_nchans == static_cast<size_t>(afInput.dims(0)));
        assert(elems == static_cast<size_t>(afInput.dims(1)));

        afArrayMinValue = af::constant(_afMinValue, _nchans, elems, afDType);
        afArrayMaxValue = af::constant(_afMaxValue, _nchans, elems, afDType);
        auto afOutput = af::clamp(afInput, afArrayMinValue, afArrayMaxValue);

        this->post2DAfArrayToNumberedOutputPorts(afOutput);
    }
};

template <>
void Clamp<double>::work(const af::array& afInput)
{
    static const af::dtype afDType = Pothos::Object(dtype).convert<af::dtype>();

    const size_t elems = this->workInfo().minElements;
    assert(0 < elems);

    if(1 == _nchans)
    {
        auto afOutput = af::clamp(afInput, _afMinValue, _afMaxValue);

        this->input(0)->consume(elems);
        this->output(0)->postBuffer(Pothos::Object(afOutput)
                                        .convert<Pothos::BufferChunk>());
    }
    else
    {
        assert(0 != _nchans);
        assert(_nchans == static_cast<size_t>(afInput.dims(0)));
        assert(elems == static_cast<size_t>(afInput.dims(1)));

        af::array afOutput(afInput.dims(0), afInput.dims(1), afDType);

        /*
         * Before ArrayFire 3.6, gfor was not thread-safe, as some
         * internal bookkeeping was stored globally. As of ArrayFire 3.6,
         * all of this stuff is thread-local, so we can take advantage of
         * it.
         */
        #if AF_CONFIG_PER_THREAD
        gfor(size_t chan, this->_nchans)
        #else
        for(size_t chan = 0; chan < this->_nchans; ++chan)
        #endif
        {
            afOutput.row(chan) = af::clamp(
                                     afInput.row(chan),
                                     _afMinValue,
                                     _afMaxValue);
        }

        this->post2DAfArrayToNumberedOutputPorts(afOutput);
    }
};

static Pothos::Block* clampFactory(
    const Pothos::DType& dtype,
    const Pothos::Object& minValue,
    const Pothos::Object& maxValue,
    size_t nchans)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new Clamp<T>(minValue.convert<T>(), maxValue.convert<T>(), nchans);

    // ArrayFire has no implementation for std::int8_t.
    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::int64_t)
    ifTypeDeclareFactory(std::uint8_t)
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    ifTypeDeclareFactory(std::uint64_t)
    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)
    // ArrayFire has no implementation for any integral complex type.

    // TODO: how does ArrayFire compare complex numbers?
    /*ifTypeDeclareFactory(std::complex<float>)
    ifTypeDeclareFactory(std::complex<double>)*/

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
}

static Pothos::BlockRegistry registerClamp(
    "/arrayfire/arith/clamp",
    Pothos::Callable(&clampFactory));

#endif