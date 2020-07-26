// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <vector>

class Pow: public OneToOneBlock
{
    public:
        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype,
            double power)
        {
            static const DTypeSupport dtypeSupport{true,true,true,true};
            validateDType(dtype, dtypeSupport);

            return new Pow(device, dtype, power);
        }

        Pow(const std::string& device,
            const Pothos::DType& dtype,
            double power
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(), // Will be set later
                dtype,
                dtype)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Pow, power));
            this->registerCall(this, POTHOS_FCN_TUPLE(Pow, setPower));

            this->registerProbe("power");
            this->registerSignal("powerChanged");

            this->setPower(power);
        }

        virtual ~Pow() {};

        double power() const
        {
            return _power;
        }

        void setPower(double power)
        {
            _power = power;

            // To disambiguate an overloaded function
            using PowType = af::array(*)(const af::array&, const double);

            _func = Pothos::Callable(PowType(af::pow));
            _func.bind(_power, 1);

            this->emitSignal("powerChanged", power);
        }

    private:
        double _power;
};

/*
 * |PothosDoc Pow
 *
 * Uses <b>af::pow</b> to calculate the power of each input element
 * to the given scalar power value.
 *
 * |category /GPU/Arith
 * |keywords exponent power
 * |factory /gpu/arith/pow(device,dtype,power)
 * |setter setPower(power)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param power[Power] The exponent value.
 * |widget DoubleSpinBox()
 * |default 0.0
 * |preview enable
 */
static Pothos::BlockRegistry registerPow(
    "/gpu/arith/pow",
    Pothos::Callable(&Pow::make));
