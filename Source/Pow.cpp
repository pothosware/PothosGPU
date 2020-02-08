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

            this->registerProbe("getPower", "powerChanged", "setPower");

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

            if(2.0 == power)
            {
                _func = Pothos::Callable(&af::pow2);
            }
            else
            {
                // To disambiguate an overloaded function
                using PowType = af::array(*)(const af::array&, const double);

                _func = Pothos::Callable(PowType(af::pow));
                _func.bind(_power, 1);
            }

            this->emitSignal("powerChanged", power);
        }

    private:
        double _power;
};
