// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cstring>
#include <string>

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cassert>
#include <string>
#include <typeinfo>

using OneToOneFunc = af::array(*)(const af::array&);

class OneToOneBatchBlock: public ArrayFireBlock
{
    public:
        //
        // Factories
        //

        static Pothos::Block* makeFromOneType(
            const OneToOneFunc& func,
            const Pothos::DType& dtype,
            const DTypeSupport& supportedTypes,
            size_t numChans)
        {
            validateDType(dtype, supportedTypes);

            return new OneToOneBatchBlock(func, dtype, dtype, numChans);
        }

        static Pothos::Block* makeFromTwoTypes(
            const OneToOneFunc& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            const DTypeSupport& supportedInputTypes,
            const DTypeSupport& supportedOutputTypes,
            size_t numChans)
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

            return new OneToOneBatchBlock(func, inputDType, outputDType, numChans);
        }

        //
        // Class implementation
        //

        OneToOneBatchBlock(
            const OneToOneFunc& func,
            const Pothos::DType& inputDType,
            const Pothos::DType& outputDType,
            size_t numChans
        ): ArrayFireBlock(),
           _func(func)
        {
            for(size_t chan = 0; chan < numChans; ++chan)
            {
                this->setupInput(chan, inputDType);
                this->setupOutput(chan, outputDType);
            }
        }

        virtual ~OneToOneBatchBlock() {}

        af::array getNumberedInputPortsAs2DAfArray()
        {
            // Assumptions:
            //  * We've already checked that all buffers are non-empty.
            //  * We only have numbered ports.
            //  * All DTypes are the same.
            const auto& inputs = this->inputs();
            assert(!inputs.empty());

            const auto dim0 = static_cast<dim_t>(inputs.size());
            const auto dim1 = static_cast<dim_t>(this->workInfo().minElements);
            const auto afDType = Pothos::Object(inputs[0]->dtype()).convert<::af_dtype>();
            const size_t rowSizeBytes = inputs[0]->dtype().size() * dim1;

            af::array ret(dim0, dim1, afDType);
            for(dim_t row = 0; row < dim0; ++row)
            {
                auto arrayRow = ret.row(row);
                std::memcpy(
                    arrayRow.device<std::uint8_t>(),
                    inputs[row]->buffer().as<const std::uint8_t*>(),
                    rowSizeBytes);
                arrayRow.unlock();

                inputs[row]->consume(dim1);
            }

            return ret;
        }

        void post2DAfArrayToNumberedOutputPorts(const af::array& afArray)
        {
            const auto& outputs = this->outputs();
            assert(outputs.size() == static_cast<size_t>(afArray.dims(0)));

            for(size_t portIndex = 0; portIndex < outputs.size(); ++portIndex)
            {
                auto outputBuffer = Pothos::Object(afArray.row(portIndex))
                                        .convert<Pothos::BufferChunk>();
                outputs[portIndex]->postBuffer(std::move(outputBuffer));
            }
        }

        void work() override
        {
            if(0 == this->workInfo().minElements)
            {
                return;
            }

            auto afInput = getNumberedInputPortsAs2DAfArray();
            const auto dim0 = afInput.dims(0);
            const auto dim1 = afInput.dims(1);

            af::array afOutput(dim0, dim1, afInput.type());
            gfor(af::seq row, dim0)
            {
                afOutput(row) = _func(afInput(row));
            }

            post2DAfArrayToNumberedOutputPorts(afOutput);
        }

    private:
        OneToOneFunc _func;
};
