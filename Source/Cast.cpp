// Copyright (c) 2019-2021 Nicholas Corgan
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

// To avoid collisions
namespace
{

static void validateCastTypes(
    const Pothos::DType& inputDType,
    const Pothos::DType& outputDType)
{
    if(isDTypeComplexFloat(inputDType) && !outputDType.isComplex())
    {
        throw Pothos::InvalidArgumentException(
                  "This block cannot perform complex to scalar conversions.");
    }
}

class CastBlock: public OneToOneBlock
{
    public:

        // This is what will be registered.
        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType)
        {
            // Validate here to avoid the ArrayFireBlock overhead.
            validateCastTypes(inputDType, outputDType);

            return new CastBlock(device, inputDType, outputDType);
        }

        CastBlock(
            const std::string& device,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(),
                inputDType,
                outputDType)
        {
        }

        void work() override
        {
            const size_t elems = this->workInfo().minElements;
            if(0 == elems)
            {
                return;
            }

            auto afOutput = this->getInputPortAsAfArray(0).as(_afOutputDType);
            this->produceFromAfArray(0, afOutput);
        }
};

/*
 * |PothosDoc Cast (GPU)
 *
 * Calls <b>af::array::as</b> on all inputs to cast to a given type.
 *
 * |category /GPU/Convert
 * |keywords array cast
 * |factory /gpu/array/cast(device,inputDType,outputDType)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param inputDType[Input Data Type] The block data type.
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param outputDType[Output Data Type] The block data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,cfloat=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerCast(
    "/gpu/array/cast",
    Pothos::Callable(&CastBlock::make));

}
