// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Functions.hpp"
#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <vector>

class Log: public OneToOneBlock
{
    public:
        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype,
            double base)
        {
            // Supports float
            static const DTypeSupport dtypeSupport{false,false,true,false};
            validateDType(dtype, dtypeSupport);

            return new Log(device, dtype, base);
        }

        Log(const std::string& device,
            const Pothos::DType& dtype,
            double base
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(), // Will be set later
                dtype,
                dtype)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Log, base));
            this->registerCall(this, POTHOS_FCN_TUPLE(Log, setBase));

            this->registerProbe("base");
            this->registerSignal("baseChanged");

            this->setBase(base);
        }

        virtual ~Log() {};

        double base() const
        {
            return _base;
        }

        void setBase(double base)
        {
            _base = base;

            if(2.0 == base)
            {
                _func = Pothos::Callable(&af::log2);
            }
            else if(10.0 == base)
            {
                _func = Pothos::Callable(&af::log10);
            }
            else
            {
                _func = Pothos::Callable(&logN);
                _func.bind(_base, 1);
            }

            this->emitSignal("baseChanged", base);
        }

    private:
        double _base;
};

/*
 * |PothosDoc Log N
 *
 * Calculates the logarithm of each value in the input stream with the
 * scalar caller-given base. Uses <b>af::log2</b> and <b>af::log10</b>
 * to optimize the log-2 and log-10 cases.
 *
 * |category /ArrayFire/Arith
 * |keywords exponent power
 * |factory /arrayfire/arith/log(device,dtype,base)
 * |setter setBase(base)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param base[Base] The logarithm base.
 * |widget DoubleSpinBox()
 * |default 10
 * |preview enable
 */
static Pothos::BlockRegistry registerLogN(
    "/arrayfire/arith/log",
    Pothos::Callable(&Log::make));
