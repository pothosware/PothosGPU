// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <cstdint>
#include <typeinfo>

template <typename T>
using MinMaxFunction = void(*)(T*, unsigned*, const af::array&);

template <typename T>
class MinMax: public ArrayFireBlock
{
    public:

        MinMax(
            const MinMaxFunction<T>& func,
            const std::string& labelName
        ):
            ArrayFireBlock(),
            _func(func),
            _labelName(labelName)
        {
            using Class = MinMax<T>;

            this->setupInput(0, MinMax<T>::dtype);

            // Unique domain because of buffer forwarding
            this->setupOutput(0, MinMax<T>::dtype, this->uid());
        }

        virtual ~MinMax() {}

        void work() override
        {
            const size_t elems = this->workInfo().minAllElements;
            if(0 == elems)
            {
                return;
            }

            auto input = this->input(0);
            auto output = this->output(0);

            auto buffer = input->takeBuffer();

            T val(0);
            unsigned idx(0);
            _func(
                &val,
                &idx,
                Pothos::Object(buffer).convert<af::array>());

            output->postLabel(
                _labelName,
                val,
                idx);

            input->consume(elems);
            output->postBuffer(std::move(buffer));
        }

    private:

        static const Pothos::DType dtype;

        MinMaxFunction<T> _func;
        std::string _labelName;
};

template <typename T>
const Pothos::DType MinMax<T>::dtype(typeid(T));

template <bool isMin>
static Pothos::Block* minMaxFactory(const Pothos::DType& dtype)
{
    static constexpr const char* labelName = isMin ? "MIN" : "MAX";

    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new MinMax<T>( \
                           (isMin ? (MinMaxFunction<T>)af::min<T> : (MinMaxFunction<T>)af::max<T>), \
                           labelName);

    // ArrayFire has no implementation for int8_t, int64_t, or uint64_t.
    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::uint8_t)
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
}

static Pothos::BlockRegistry registerMin(
    "/arrayfire/algorithm/min",
    Pothos::Callable(&minMaxFactory<true>));

static Pothos::BlockRegistry registerMax(
    "/arrayfire/algorithm/max",
    Pothos::Callable(&minMaxFactory<false>));
