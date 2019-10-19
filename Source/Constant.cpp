// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cstdint>
#include <typeinfo>

template <typename T>
class Constant: public ArrayFireBlock
{
    public:

        Constant(T constant):
            ArrayFireBlock(),
            _constant(constant),
            _afDType(Pothos::Object(Constant<T>::dtype).convert<af::dtype>())
        {
            using Class = Constant<T>;

            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getConstant));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setConstant));
            this->setupOutput(0, Constant<T>::dtype);
        }

        virtual ~Constant() {}

        void work() override
        {
            // Since we post all buffers but don't have an input size
            // to match.
            static constexpr dim_t OutputBufferSize = 1024;

            this->postAfArray(
                0,
                af::constant(_constant, OutputBufferSize, _afDType));
        }

        T getConstant() const
        {
            return _constant;
        }

        void setConstant(const T& constant)
        {
            _constant = constant;
        }

    private:

        static const Pothos::DType dtype;

        T _constant;
        af::dtype _afDType;
};

template <typename T>
const Pothos::DType Constant<T>::dtype(typeid(T));

static Pothos::Block* constantFactory(const Pothos::DType& dtype)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new Constant<T>(T(0));

    // ArrayFire has no implementation for std::int8_t or either complex type.
    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::int64_t)
    ifTypeDeclareFactory(std::uint8_t)
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    ifTypeDeclareFactory(std::uint64_t)
    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
}

static Pothos::BlockRegistry registerConstant(
    "/arrayfire/data/constant",
    Pothos::Callable(&constantFactory));
