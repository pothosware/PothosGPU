// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cassert>
#include <cstring>
#include <string>
#include <typeinfo>

// TODO:
//  * See if different backends have different casting support.
//  * If this is the case, automatically set backend or error out
//    based on given DTypes.
class CastBlock: public OneToOneBlock
{
    public:

        // This is what will be registered.
        static Pothos::Block* make(
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            size_t nchans)
        {
            return new CastBlock(inputDType, outputDType, nchans);
        }

        CastBlock(
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            size_t nchans
        ):
            OneToOneBlock(
                Pothos::Callable(),
                inputDType,
                outputDType,
                nchans)
        {
        }

        void work(const af::array& afArray) override
        {
            const size_t elems = this->workInfo().minElements;
            assert(elems > 0);

            auto afOutput = afArray.as(_afOutputDType);
            if(1 == _nchans)
            {
                this->input(0)->consume(elems);
                this->output(0)->postBuffer(Pothos::Object(afOutput)
                                                .convert<Pothos::BufferChunk>());
            }
            else
            {
                assert(0 != _nchans);

                this->post2DAfArrayToNumberedOutputPorts(afOutput);
            }
        }
};

static Pothos::BlockRegistry registerCast(
    "/arrayfire/stream/cast",
    Pothos::Callable(&CastBlock::make));
