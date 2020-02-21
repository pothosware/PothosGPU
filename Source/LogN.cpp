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
            // Supports float, complex
            static const DTypeSupport dtypeSupport{false,false,true,true};
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
            this->registerCall(this, POTHOS_FCN_TUPLE(Log, getBase));
            this->registerCall(this, POTHOS_FCN_TUPLE(Log, setBase));

            this->registerProbe("getBase");
            this->registerSignal("baseChanged");

            this->setBase(base);
        }

        virtual ~Log() {};

        double getBase() const
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

static Pothos::BlockRegistry registerLogN(
    "/arrayfire/arith/log",
    Pothos::Callable(&Log::make));
