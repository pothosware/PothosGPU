// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cassert>
#include <string>
#include <typeinfo>

using OneToOneFunc = af::array(*)(const af::array&);

template <typename In, typename Out>
class OneToOneBlock: public ArrayFireBlock
{
    public:
        using InType = In;
        using OutType = Out;

        OneToOneBlock(const OneToOneFunc& func):
            ArrayFireBlock(),
            _func(func)
        {
            this->setupInput(0, Pothos::DType(typeid(InType)));
            this->setupOutput(0, Pothos::DType(typeid(OutType)));
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

            this->getInput(0)->consume(elems);
            this->postAfArray(0, outputAfArray);
        }

    private:
        OneToOneFunc _func;
};
