// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cstdint>
#include <typeinfo>

using namespace PothosGPU;

// To avoid collisions
namespace
{

template <typename T>
class Constant: public ArrayFireBlock
{
    public:

        using Class = Constant<T>;

        Constant(
            const std::string& device,
            T constant,
            size_t dtypeDims
        ):
            ArrayFireBlock(device),
            _afDType(Pothos::Object(Class::dtype).convert<af::dtype>())
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, constant));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setConstant));
            this->setupOutput(
                0,
                Pothos::DType::fromDType(Class::dtype, dtypeDims));

            this->registerProbe("constant");
            this->registerSignal("constantChanged");

            this->setConstant(constant);
        }

        virtual ~Constant() {}

        void work() override
        {
            const auto elems = this->workInfo().minElements;
            if(0 == elems)
            {
                return;
            }
            
            this->produceFromAfArray(
                0,
                af::constant(_constant, elems, _afDType));
        }

        T constant() const
        {
            return PothosToAF<T>::from(_constant);
        }

        void setConstant(const T& constant)
        {
            _constant = PothosToAF<T>::to(constant);
            this->emitSignal("constantChanged", constant);
        }

    private:

        static const Pothos::DType dtype;

        typename PothosToAF<T>::type _constant;
        af::dtype _afDType;
};

template <typename T>
const Pothos::DType Constant<T>::dtype(typeid(T));

static Pothos::Block* constantFactory(
    const std::string& device,
    const Pothos::DType& dtype,
    const Pothos::Object& constant)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new Constant<T>(device, constant.convert<T>(), dtype.dimension());

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
    // ArrayFire does not support any integral complex numbers.
    ifTypeDeclareFactory(std::complex<float>)
    ifTypeDeclareFactory(std::complex<double>)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
}

/*
 * |PothosDoc Constant Source (GPU)
 *
 * Calls <b>af::constant</b> to fill all outgoing buffers with a given value.
 *
 * |category /GPU/Sources
 * |keywords data constant
 * |factory /gpu/data/constant(device,dtype,constant)
 * |setter setConstant(constant)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The block data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param constant[Constant] Which constant to fill the buffer with.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 */
static Pothos::BlockRegistry registerConstant(
    "/gpu/data/constant",
    Pothos::Callable(&constantFactory));

}
