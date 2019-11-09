// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

template <typename T>
using AfArrayScalarOp = af::array(*)(
                            const af::array&,
                            const typename PothosToAF<T>::type&);

template <typename T>
class ScalarOpBlock: public OneToOneBlock
{
    public:

        using Class = ScalarOpBlock<T>;

        ScalarOpBlock(
            const AfArrayScalarOp<T>& func,
            const Pothos::DType& dtype,
            T scalar,
            size_t numChans);

        virtual ~ScalarOpBlock();

        T getScalar() const;

        void setScalar(const T& scalar);

        void work() override;

    private:
        typename PothosToAF<T>::type _scalar;
};
