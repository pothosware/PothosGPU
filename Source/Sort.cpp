// Copyright (c) 2020-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <vector>

// To avoid collisions
namespace
{

// To disambiguate
using SortFcn = af::array(*)(const af::array&, const unsigned, const bool);

class Sort: public OneToOneBlock
{
    public:
        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype)
        {
            // Supports all but complex
            static const DTypeSupport dtypeSupport{true,true,true,false};
            validateDType(dtype, dtypeSupport);

            return new Sort(device, dtype);
        }

        Sort(const std::string& device,
             const Pothos::DType& dtype
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(SortFcn(af::sort)).bind(0U, 1),
                dtype,
                dtype)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Sort, isAscending));
            this->registerCall(this, POTHOS_FCN_TUPLE(Sort, setIsAscending));

            this->registerProbe("isAscending");
            this->registerSignal("isAscendingChanged");

            // Set here instead of class instantiation to send signal
            this->setIsAscending(true);
        }

        virtual ~Sort() {};

        double isAscending() const
        {
            return _isAscending;
        }

        void setIsAscending(bool isAscending)
        {
            _isAscending = isAscending;
            _func.bind(_isAscending, 2);
            
            this->emitSignal("isAscendingChanged", isAscending);
        }

    private:
        bool _isAscending;
};

/*
 * |PothosDoc Sort (GPU)
 *
 * |category /GPU/Stream
 * |factory /gpu/algorithm/sort(device,dtype)
 * |setter setIsAscending(isAscending)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The output's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param isAscending[Ascending?] Whether to sort by ascending or descending.
 * |widget ToggleSwitch(on="True", off="False")
 * |default true
 * |preview enable
 */
static Pothos::BlockRegistry registerSort(
    "/gpu/algorithm/sort",
    Pothos::Callable(&Sort::make));

}
