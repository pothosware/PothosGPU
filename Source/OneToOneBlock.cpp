// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cassert>
#include <string>
#include <typeinfo>

using OneToOneFunc = af::array(*)(const af::array&);

class OneToOneBlock: public ArrayFireBlock
{
    public:
        //
        // Factories
        //

        static Pothos::Block* makeFromOneType(
            const OneToOneFunc& func,
            const Pothos::DType& dtype,
            const DTypeSupport& supportedTypes)
        {
            validateDType(dtype, supportedTypes);

            return new OneToOneBlock(func, dtype, dtype);
        }

        static Pothos::Block* makeFromTwoTypes(
            const OneToOneFunc& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            const DTypeSupport& supportedInputTypes,
            const DTypeSupport& supportedOutputTypes)
        {
            validateDType(inputDType, supportedInputTypes);
            validateDType(outputDType, supportedOutputTypes);

            if(isDTypeComplexFloat(inputDType) && isDTypeFloat(outputDType))
            {
                validateComplexAndFloatTypesMatch(
                    inputDType,
                    outputDType);
            }
            else if(isDTypeFloat(inputDType) && isDTypeComplexFloat(outputDType))
            {
                validateComplexAndFloatTypesMatch(
                    outputDType,
                    inputDType);
            }

            return new OneToOneBlock(func, inputDType, outputDType);
        }

        //
        // Class implementation
        //

        OneToOneBlock(
            const OneToOneFunc& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType
        ): ArrayFireBlock(),
           _func(func)
        {
            this->setupInput(0, inputDType);
            this->setupOutput(0, outputDType);
        }

        virtual ~OneToOneBlock() {}

        void work() override
        {
            const size_t elems = this->workInfo().minAllElements;

            if(0 == elems)
            {
                return;
            }

            auto inputAfArray = this->getInputPortAsAfArray(0);
            assert(elems == inputAfArray.elements());

            auto outputAfArray = _func(inputAfArray);

            this->input(0)->consume(elems);
            this->postAfArray(0, outputAfArray);
        }

    private:
        OneToOneFunc _func;
};

//
// af/arith.h
//

static Pothos::BlockRegistry registerAbs(
    "/arrayfire/abs",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::abs, 0)
        .bind<DTypeSupport>({true, false, true, true}, 2));

static Pothos::BlockRegistry registerArg(
    "/arrayfire/arg",
    Pothos::Callable(&OneToOneBlock::makeFromTwoTypes)
        .bind<OneToOneFunc>(&af::arg, 0)
        .bind<DTypeSupport>({false, false, false, true}, 3)
        .bind<DTypeSupport>({false, false, true, false}, 4));
