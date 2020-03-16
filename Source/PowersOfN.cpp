// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <vector>

class PowN: public OneToOneBlock
{
    public:
        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype,
            double base)
        {
            // Supports float, complex
            static const DTypeSupport dtypeSupport{false,false,true,true};
            validateDType(dtype, dtypeSupport);

            return new PowN(device, dtype, base);
        }

        PowN(const std::string& device,
             const Pothos::DType& dtype,
             double base
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(), // Will be set later
                dtype,
                dtype)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(PowN, base));
            this->registerCall(this, POTHOS_FCN_TUPLE(PowN, setBase));

            this->registerProbe("base");
            this->registerSignal("baseChanged");

            this->setBase(base);
        }

        virtual ~PowN() {};

        double base() const
        {
            return _base;
        }

        void setBase(double base)
        {
            _base = base;

            // To disambiguate an overloaded function
            using PowType = af::array(*)(const double, const af::array&);

            if(2.0 == base)
            {
                _func = Pothos::Callable(af::pow2);
            }
            else
            {
                _func = Pothos::Callable(PowType(af::pow));
                _func.bind(_base, 0);
            }

            this->emitSignal("baseChanged", base);
        }

    private:
        double _base;
};

/*
 * |PothosDoc Powers of N
 *
 * Uses <b>af::pow</b> to calculate a scalar base to the base of each
 * input element. Uses <b>af::pow2</b> to optimize the case where
 * <b>base = 2</b>.
 *
 * |category /ArrayFire/Arith
 * |keywords exponent base pow2
 * |factory /arrayfire/arith/powN(device,dtype,base)
 * |setter setBase(base)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param base[Base] The base value.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 */
static Pothos::BlockRegistry registerPow(
    "/arrayfire/arith/powN",
    Pothos::Callable(&PowN::make));
