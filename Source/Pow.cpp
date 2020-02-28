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
            // Supports float, complex
            static const DTypeSupport dtypeSupport{false,false,true,true};
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
            this->registerCall(this, POTHOS_FCN_TUPLE(Pow, getPower));
            this->registerCall(this, POTHOS_FCN_TUPLE(Pow, setPower));

            this->registerProbe("getPower");
            this->registerSignal("powerChanged");

            this->setPower(power);
        }

        virtual ~Pow() {};

        double getPower() const
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

static Pothos::BlockRegistry registerLogN(
    "/arrayfire/arith/pow",
    Pothos::Callable(&Pow::make));
