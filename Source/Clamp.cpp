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

#include <cstdint>
#include <iostream>
#include <typeinfo>

template <typename T>
class Clamp: public OneToOneBlock
{
    public:
        using Class = Clamp<T>;
        using AFType = typename PothosToAF<T>::type;

        static const Pothos::DType dtype;

        Clamp(
            const std::string& device,
            const T& minValue,
            const T& maxValue,
            size_t dtypeDims
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(),
                Pothos::DType::fromDType(Class::dtype, dtypeDims),
                Pothos::DType::fromDType(Class::dtype, dtypeDims))
        {
            validateMinMax(minValue, maxValue);

            _afMinValue = PothosToAF<T>::to(minValue);
            _afMaxValue = PothosToAF<T>::to(maxValue);

            this->registerCall(this, POTHOS_FCN_TUPLE(Class, minValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setMinValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, maxValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setMaxValue));

            this->registerProbe("minValue");
            this->registerProbe("maxValue");

            this->registerSignal("minValueChanged");
            this->registerSignal("maxValueChanged");
        }

        virtual ~Clamp() = default;

        T minValue() const
        {
            return PothosToAF<T>::from(_afMinValue);
        }

        void setMinValue(const T& minValue)
        {
            validateMinMax(minValue, maxValue());

            _afMinValue = PothosToAF<T>::to(minValue);
            this->emitSignal("minValueChanged", minValue);
        }

        T maxValue() const
        {
            return PothosToAF<T>::from(_afMaxValue);
        }

        void setMaxValue(const T& maxValue)
        {
            validateMinMax(minValue(), maxValue);

            _afMaxValue = PothosToAF<T>::to(maxValue);
            this->emitSignal("maxValueChanged", maxValue);
        }

        void work() override;

    private:
        AFType _afMinValue;
        AFType _afMaxValue;

        void validateMinMax(const T& min, const T& max)
        {
            if(min > max)
            {
                throw Pothos::InvalidArgumentException(
                          "minValue must be < maxValue",
                          Poco::format(
                              "%s > %s",
                              Poco::NumberFormatter::format(min),
                              Poco::NumberFormatter::format(max)));
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
void Clamp<T>::work()
{
    const auto elems = this->workInfo().minElements;
    if(0 == elems)
    {
        return;
    }

    static const af::dtype afDType = Pothos::Object(dtype).convert<af::dtype>();

    auto afArrayMinValue = af::constant(_afMinValue, elems, afDType);
    auto afArrayMaxValue = af::constant(_afMaxValue, elems, afDType);

    auto afInput = this->getInputPortAsAfArray(0);
    auto afOutput = af::clamp(afInput, afArrayMinValue, afArrayMaxValue);
    this->produceFromAfArray(0, afOutput);
};

template <>
void Clamp<double>::work()
{
    const auto elems = this->workInfo().minElements;
    if(0 == elems)
    {
        return;
    }

    auto afInput = this->getInputPortAsAfArray(0);
    auto afOutput = af::clamp(afInput, _afMinValue, _afMaxValue);
    this->produceFromAfArray(0, afOutput);
};

/*
 * |PothosDoc Clamp (GPU)
 *
 * Calls <b>af::clamp</b> on all inputs with given minimum and maximum values.
 *
 * |category /GPU/Arith
 * |category /Stream/GPU
 * |keywords array arith clamp min max
 * |factory /gpu/arith/clamp(device,dtype,minValue,maxValue)
 * |setter setMinValue(minValue)
 * |setter setMaxValue(maxValue)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param minValue[Min Value]
 * |default 0
 * |preview enable
 *
 * |param maxValue[Max Value]
 * |default 10
 * |preview enable
 */
static Pothos::Block* clampFactory(
    const std::string& device,
    const Pothos::DType& dtype,
    const Pothos::Object& minValue,
    const Pothos::Object& maxValue)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new Clamp<T>(device, minValue.convert<T>(), maxValue.convert<T>(), dtype.dimension());

    ifTypeDeclareFactory(char)
    ifTypeDeclareFactory(short)
    ifTypeDeclareFactory(int)
    ifTypeDeclareFactory(long long)
    ifTypeDeclareFactory(unsigned char)
    ifTypeDeclareFactory(unsigned short)
    ifTypeDeclareFactory(unsigned)
    ifTypeDeclareFactory(unsigned long long)
    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)
    // ArrayFire has no implementation for any integral complex type.

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
}

static Pothos::BlockRegistry registerClamp(
    "/gpu/arith/clamp",
    Pothos::Callable(&clampFactory));
