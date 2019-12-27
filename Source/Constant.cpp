// Copyright (c) 2019 Nicholas Corgan
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
class Constant: public ArrayFireBlock
{
    public:

        using Class = Constant<T>;

        Constant(T constant):
            ArrayFireBlock(),
            _constant(PothosToAF<T>::to(constant)),
            _afDType(Pothos::Object(Class::dtype).convert<af::dtype>())
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, getConstant));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setConstant));
            this->setupOutput(0, Class::dtype);
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
            return PothosToAF<T>::from(_constant);
        }

        void setConstant(const T& constant)
        {
            _constant = PothosToAF<T>::to(constant);
        }

    private:

        static const Pothos::DType dtype;

        typename PothosToAF<T>::type _constant;
        af::dtype _afDType;
};

template <typename T>
const Pothos::DType Constant<T>::dtype(typeid(T));

static Pothos::Block* constantFactory(
    const Pothos::DType& dtype,
    const Pothos::Object& constant)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new Constant<T>(constant.convert<T>());

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
    ifTypeDeclareFactory(std::complex<float>)
    ifTypeDeclareFactory(std::complex<double>)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
}

/*
 * |PothosDoc Constant
 *
 * Calls <b>af::constant</b> on all inputs. to fill buffers with a given
 * constant. This is potentially accelerated using one of the following
 * implementations by priority (based on availability of hardware
 * and underlying libraries).
 * <ol>
 * <li>CUDA (if GPU present)</li>
 * <li>OpenCL (if GPU present)</li>
 * <li>Standard C++ (if no GPU present)</li>
 * </ol>
 *
 * |category /ArrayFire/Data
 * |keywords data constant
 * |factory /arrayfire/data/constant(dtype,constant)
 *
 * |param dtype(Data Type) The block data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1)
 * |default "float64"
 * |preview disable
 *
 * |param constant(Constant) Which constant to fill the buffer with.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 */
static Pothos::BlockRegistry registerConstant(
    "/arrayfire/data/constant",
    Pothos::Callable(&constantFactory));
