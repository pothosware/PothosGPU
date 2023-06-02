// Copyright (c) 2021,2023 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

//
// Block class
//

class ModF: public ArrayFireBlock
{
    public:
        static const std::string IntPortName;
        static const std::string FracPortName;

        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype)
        {
            static const DTypeSupport FloatOnlyDTypeSupport{false,false,true,false};
            validateDType(dtype, FloatOnlyDTypeSupport);

            return new ModF(device, dtype);
        }

        ModF(
            const std::string& device,
            const Pothos::DType& dtype
        ):
            ArrayFireBlock(device)
        {
            this->setupInput(0, dtype, _domain);
            this->setupOutput(IntPortName, dtype, _domain);
            this->setupOutput(FracPortName, dtype, _domain);
        }

        virtual ~ModF() = default;

        void work() override
        {
            const size_t elems = this->workInfo().minAllElements;
            if(0 == elems)
            {
                return;
            }

            auto afInput = this->getInputPortAsAfArray(0);
            auto afInt = af::trunc(afInput);
            auto afFrac = afInput - afInt;

            this->produceFromAfArray(IntPortName, afInt);
            this->produceFromAfArray(FracPortName, afFrac);
        }
};

const std::string ModF::IntPortName = "int";
const std::string ModF::FracPortName = "frac";

//
// Factory
//

/*
 * |PothosDoc Decompose Floats (GPU)
 *
 * Separates the integral and fractional components of each floating-point
 * input element.
 *
 * |category /GPU/Arith
 * |category /Math/GPU
 * |keywords math/fractional
 * |factory /gpu/arith/modf(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The block data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerSplitComplex(
    "/gpu/arith/modf",
    Pothos::Callable(&ModF::make));
