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

using SingleOutputFunc = af::array(*)(const dim_t, const af::dtype);

class SingleOutputSource: public ArrayFireBlock
{
    public:
        //
        // Factories
        //

        static Pothos::Block* make(
            const SingleOutputFunc& func,
            const Pothos::DType& dtype,
            const DTypeSupport& supportedTypes)
        {
            validateDType(dtype, supportedTypes);

            return new SingleOutputSource(func, dtype);
        }

        //
        // Class implementation
        //

        // Since we post all buffers but don't have an input size
        // to match.
        static constexpr dim_t OutputBufferSize = 1024;

        SingleOutputSource(
            const SingleOutputFunc& func,
            const Pothos::DType& dtype
        ): ArrayFireBlock(),
           _func(func),
           _afDType(Pothos::Object(dtype).convert<af::dtype>())
        {
            this->setupOutput(0, dtype);
        }

        virtual ~SingleOutputSource() {}

        void work() override
        {
            this->postAfArray(
                0,
                _func(OutputBufferSize, _afDType));
        }

    private:
        SingleOutputFunc _func;
        af::dtype _afDType;
};

//
// af/data.h
//

static Pothos::BlockRegistry registerRandU(
    "/arrayfire/data/randu",
    Pothos::Callable(&SingleOutputSource::make)
        .bind<SingleOutputFunc>(&af::randu, 0)
        .bind<DTypeSupport>({true, true, true, true}, 2));

static Pothos::BlockRegistry registerRandN(
    "/arrayfire/data/randn",
    Pothos::Callable(&SingleOutputSource::make)
        .bind<SingleOutputFunc>(&af::randn, 0)
        .bind<DTypeSupport>({true, true, true, true}, 2));

static Pothos::BlockRegistry registerIdentity(
    "/arrayfire/data/identity",
    Pothos::Callable(&SingleOutputSource::make)
        .bind<SingleOutputFunc>(&af::identity, 0)
        .bind<DTypeSupport>({true, true, true, true}, 2));
