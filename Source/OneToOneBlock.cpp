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
    "/arrayfire/arith/abs",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::abs, 0)
        .bind<DTypeSupport>({true, false, true, true}, 2));

static Pothos::BlockRegistry registerArg(
    "/arrayfire/arith/arg",
    Pothos::Callable(&OneToOneBlock::makeFromTwoTypes)
        .bind<OneToOneFunc>(&af::arg, 0)
        .bind<DTypeSupport>({false, false, false, true}, 3)
        .bind<DTypeSupport>({false, false, true, false}, 4));

static Pothos::BlockRegistry registerRound(
    "/arrayfire/arith/round",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::round, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerTrunc(
    "/arrayfire/arith/trunc",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::trunc, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerFloor(
    "/arrayfire/arith/floor",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::floor, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerCeil(
    "/arrayfire/arith/ceil",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::ceil, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerSin(
    "/arrayfire/arith/sin",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::sin, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerCos(
    "/arrayfire/arith/cos",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::cos, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerTan(
    "/arrayfire/arith/tan",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::tan, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerASin(
    "/arrayfire/arith/asin",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::asin, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerACos(
    "/arrayfire/arith/acos",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::acos, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerATan(
    "/arrayfire/arith/atan",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::atan, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerComplex(
    "/arrayfire/arith/complex",
    Pothos::Callable(&OneToOneBlock::makeFromTwoTypes)
        .bind<OneToOneFunc>(&af::complex, 0)
        .bind<DTypeSupport>({false, false, true, false}, 3)
        .bind<DTypeSupport>({false, false, false, true}, 4));

static Pothos::BlockRegistry registerReal(
    "/arrayfire/arith/real",
    Pothos::Callable(&OneToOneBlock::makeFromTwoTypes)
        .bind<OneToOneFunc>(&af::real, 0)
        .bind<DTypeSupport>({false, false, false, true}, 3)
        .bind<DTypeSupport>({false, false, true, false}, 4));

static Pothos::BlockRegistry registerImag(
    "/arrayfire/arith/imag",
    Pothos::Callable(&OneToOneBlock::makeFromTwoTypes)
        .bind<OneToOneFunc>(&af::imag, 0)
        .bind<DTypeSupport>({false, false, false, true}, 3)
        .bind<DTypeSupport>({false, false, true, false}, 4));

static Pothos::BlockRegistry registerConjg(
    "/arrayfire/arith/conjg",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::conjg, 0)
        .bind<DTypeSupport>({false, false, false, true}, 2));

static Pothos::BlockRegistry registerSinH(
    "/arrayfire/arith/sinh",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::sinh, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerCosH(
    "/arrayfire/arith/cosh",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::cosh, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerTanH(
    "/arrayfire/arith/tanh",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::tanh, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerASinH(
    "/arrayfire/arith/asinh",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::asinh, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerACosH(
    "/arrayfire/arith/acosh",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::acosh, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerATanH(
    "/arrayfire/arith/atanh",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::atanh, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerPow2(
    "/arrayfire/arith/pow2",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::pow2, 0)
        .bind<DTypeSupport>({true, true, true, true}, 2));

static Pothos::BlockRegistry registerSigmoid(
    "/arrayfire/arith/sigmoid",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::sigmoid, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerExp(
    "/arrayfire/arith/exp",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::exp, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerExpM1(
    "/arrayfire/arith/expm1",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::expm1, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerErF(
    "/arrayfire/arith/erf",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::erf, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerErFC(
    "/arrayfire/arith/erfc",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::erfc, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerLog(
    "/arrayfire/arith/log",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::log, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerLog1P(
    "/arrayfire/arith/log1p",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::log1p, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerLog10(
    "/arrayfire/arith/log10",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::log10, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerLog2(
    "/arrayfire/arith/log2",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::log2, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerSqRt(
    "/arrayfire/arith/sqrt",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::sqrt, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerCbRt(
    "/arrayfire/arith/cbrt",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::cbrt, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerFactorial(
    "/arrayfire/arith/factorial",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::factorial, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerTGamma(
    "/arrayfire/arith/tgamma",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::tgamma, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));

static Pothos::BlockRegistry registerLGamma(
    "/arrayfire/arith/lgamma",
    Pothos::Callable(&OneToOneBlock::makeFromOneType)
        .bind<OneToOneFunc>(&af::lgamma, 0)
        .bind<DTypeSupport>({false, false, true, false}, 2));
