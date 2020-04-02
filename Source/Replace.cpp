// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cstdint>
#include <typeinfo>

template <typename T>
class Replace: public ArrayFireBlock
{
    public:

        using Class = Replace<T>;

        Replace(
            const std::string& device,
            T findValue,
            T replaceValue,
            size_t dtypeDims
        ):
            ArrayFireBlock(device),
            _afDType(Pothos::Object(Class::dtype).convert<af::dtype>())
        {
            this->setupInput(
                0,
                Pothos::DType::fromDType(Class::dtype, dtypeDims));
            this->setupOutput(
                0,
                Pothos::DType::fromDType(Class::dtype, dtypeDims));

            this->registerCall(this, POTHOS_FCN_TUPLE(Class, findValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setFindValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, replaceValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setReplaceValue));

            this->registerProbe("findValue");
            this->registerSignal("findValueChanged");
            this->setFindValue(findValue);

            this->registerProbe("replaceValue");
            this->registerSignal("replaceValueChanged");
            this->setReplaceValue(replaceValue);
        }

        virtual ~Replace() {}

        T findValue() const
        {
            return PothosToAF<T>::from(_findValue);
        }

        void setFindValue(const T& findValue)
        {
            _findValue = PothosToAF<T>::to(findValue);
            this->emitSignal("findValueChanged", findValue);
        }

        T replaceValue() const
        {
            return PothosToAF<T>::from(_replaceValue);
        }

        void setReplaceValue(const T& replaceValue)
        {
            _replaceValue = PothosToAF<T>::to(replaceValue);
            this->emitSignal("replaceValueChanged", replaceValue);
        }

        void work() override;

    private:

        static const Pothos::DType dtype;

        typename PothosToAF<T>::type _findValue;
        typename PothosToAF<T>::type _replaceValue;
        af::dtype _afDType;
};

template <typename T>
const Pothos::DType Replace<T>::dtype(typeid(T));

//
// ArrayFire has two versions of af::replace: a normal one that takes in an af::array
// containing the value, and a special one for double that takes the value in directly.
//

template <typename T>
void Replace<T>::work()
{
    if(0 == this->workInfo().minElements)
    {
        return;
    }

    this->configArrayFire();

    auto afArray = this->getInputPortAsAfArray(0);
    auto afCond = af::array(afArray == _findValue);

    // af::replace operates in place.
    af::replace(
        afArray,
        afCond,
        af::constant(_replaceValue, afArray.elements(), _afDType));

    this->produceFromAfArray(0, afArray);
}

template <>
void Replace<double>::work()
{
    if(0 == this->workInfo().minElements)
    {
        return;
    }

    this->configArrayFire();

    auto afArray = this->getInputPortAsAfArray(0);
    auto afCond = af::array(afArray == _findValue);

    // af::replace operates in place.
    af::replace(afArray, afCond, _replaceValue);

    this->produceFromAfArray(0, afArray);
}

//
// Factory/Registration
//

static Pothos::Block* replaceFactory(
    const std::string& device,
    const Pothos::DType& dtype,
    const Pothos::Object& findValue,
    const Pothos::Object& replaceValue)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new Replace<T>(device, findValue.convert<T>(), replaceValue.convert<T>(), dtype.dimension());

    // ArrayFire has no implementation for std::int8_t.
    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::uint8_t)
    // This function does not support 64-bit integral numbers.
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    // This function does not support 64-bit integral numbers.
    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)
    // ArrayFire does not support any integral complex numbers.
    ifTypeDeclareFactory(std::complex<float>)
    ifTypeDeclareFactory(std::complex<double>)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
}

/*
 * |PothosDoc Replace
 *
 * Calls <b>af::replace</b> to replace all elements of a given buffer of
 * a given value with another value.
 *
 * |category /ArrayFire/Stream
 * |keywords data find
 * |factory /arrayfire/data/replace(device,dtype,findValue,replaceValue)
 * |setter setFindValue(findValue)
 * |setter setReplaceValue(replaceValue)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param dtype[Data Type] The block data type.
 * |widget DTypeChooser(int16=1,int32=1,uint16=1,uint32=1,float=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param findValue[Find Value] Which value to replace.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 *
 * |param replaceValue[Replace Value] The output value.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 */
static Pothos::BlockRegistry registerReplace(
    "/arrayfire/data/replace",
    Pothos::Callable(&replaceFactory));