// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Functions.hpp"
#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>

#include <arrayfire.h>

#include <cstdint>
#include <typeinfo>

//
// Test classes
//

template <typename T>
class CombineComplex: public ArrayFireBlock
{
    public:
        using ComplexType = std::complex<T>;

        CombineComplex(
            const std::string& device,
            size_t dtypeDimensions
        ):
            ArrayFireBlock(device)
        {
            this->setupInput(
                "re",
                Pothos::DType(typeid(T), dtypeDimensions),
                _domain);
            this->setupInput(
                "im",
                Pothos::DType(typeid(T), dtypeDimensions),
                _domain);

            this->setupOutput(
                0,
                Pothos::DType(typeid(ComplexType), dtypeDimensions),
                _domain);
        }

        virtual ~CombineComplex() = default;

        void work() override
        {
            const size_t elems = this->workInfo().minAllElements;
            if(0 == elems)
            {
                return;
            }

            auto afReal = this->getInputPortAsAfArray("re");
            auto afImag = this->getInputPortAsAfArray("im");
            
            this->produceFromAfArray(0, af::complex(afReal, afImag));
        }
};

template <typename T>
class SplitComplex: public ArrayFireBlock
{
    public:
        using ComplexType = std::complex<T>;

        SplitComplex(
            const std::string& device,
            size_t dtypeDimensions
        ):
            ArrayFireBlock(device)
        {
            this->setupInput(
                0,
                Pothos::DType(typeid(ComplexType), dtypeDimensions),
                _domain);

            this->setupOutput(
                "re",
                Pothos::DType(typeid(T), dtypeDimensions),
                _domain);
            this->setupOutput(
                "im",
                Pothos::DType(typeid(T), dtypeDimensions),
                _domain);
        }

        virtual ~SplitComplex() = default;

        void work() override
        {
            const size_t elems = this->workInfo().minAllElements;
            if(0 == elems)
            {
                return;
            }

            auto afInput = this->getInputPortAsAfArray(0);
            this->produceFromAfArray("re", af::real(afInput));
            this->produceFromAfArray("im", af::imag(afInput));
        }
};

template <typename T>
class PolarToComplex: public ArrayFireBlock
{
    public:
        using ComplexType = std::complex<T>;

        PolarToComplex(
            const std::string& device,
            size_t dtypeDimensions
        ):
            ArrayFireBlock(device)
        {
            this->setupInput(
                "mag",
                Pothos::DType(typeid(T), dtypeDimensions),
                _domain);
            this->setupInput(
                "phase",
                Pothos::DType(typeid(T), dtypeDimensions),
                _domain);

            this->setupOutput(
                0,
                Pothos::DType(typeid(ComplexType), dtypeDimensions),
                _domain);
        }

        virtual ~PolarToComplex() = default;

        void work() override
        {
            const size_t elems = this->workInfo().minAllElements;
            if(0 == elems)
            {
                return;
            }

            auto afMag = this->getInputPortAsAfArray("mag");
            auto afPhase = this->getInputPortAsAfArray("phase");
            
            this->produceFromAfArray(0, polarToComplex(afMag, afPhase));
        }
};

template <typename T>
class ComplexToPolar: public ArrayFireBlock
{
    public:
        using ComplexType = std::complex<T>;

        ComplexToPolar(
            const std::string& device,
            size_t dtypeDimensions
        ):
            ArrayFireBlock(device)
        {
            this->setupInput(
                0,
                Pothos::DType(typeid(ComplexType), dtypeDimensions),
                _domain);

            this->setupOutput(
                "mag",
                Pothos::DType(typeid(T), dtypeDimensions),
                _domain);
            this->setupOutput(
                "phase",
                Pothos::DType(typeid(T), dtypeDimensions),
                _domain);
        }

        virtual ~ComplexToPolar() = default;

        void work() override
        {
            const size_t elems = this->workInfo().minAllElements;
            if(0 == elems)
            {
                return;
            }

            auto afInput = this->getInputPortAsAfArray(0);
            this->produceFromAfArray("mag", af::abs(afInput));
            this->produceFromAfArray("phase", af::arg(afInput));
        }
};

//
// Factories
//

static Pothos::Block* combineComplexFactory(
    const std::string& device,
    const Pothos::DType& dtype)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new CombineComplex<T>(device,dtype.dimension());

    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
    #undef ifTypeDeclareFactory
}

static Pothos::Block* splitComplexFactory(
    const std::string& device,
    const Pothos::DType& dtype)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new SplitComplex<T>(device,dtype.dimension());

    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
    #undef ifTypeDeclareFactory
}

static Pothos::Block* polarToComplexFactory(
    const std::string& device,
    const Pothos::DType& dtype)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new PolarToComplex<T>(device,dtype.dimension());

    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
    #undef ifTypeDeclareFactory
}

static Pothos::Block* complexToPolarFactory(
    const std::string& device,
    const Pothos::DType& dtype)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new ComplexToPolar<T>(device,dtype.dimension());

    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
              "Unsupported type",
              dtype.name());
    #undef ifTypeDeclareFactory
}

//
// Block registries
//

/*
 * |PothosDoc Combine Complex (GPU)
 *
 * Calls <b>af::complex</b> on the inputs of the <b>"re"</b> and <b>"im"</b> ports
 * and outputs the combined results.
 *
 * |category /GPU/Convert
 * |keywords arith complex real imag imaginary
 * |factory /gpu/arith/combine_complex(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The block data type. The output type will be the complex form of this type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerCombineComplex(
    "/gpu/arith/combine_complex",
    Pothos::Callable(&combineComplexFactory));

/*
 * |PothosDoc Split Complex (GPU)
 *
 * Calls <b>af::real</b> and <b>af::imag</b> on all inputs and outputs results
 * in "re" and "im" output channels.
 *
 * |category /GPU/Convert
 * |keywords arith complex real imag imaginary
 * |factory /gpu/arith/split_complex(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The block data type. The input type will be the complex form of this type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerSplitComplex(
    "/gpu/arith/split_complex",
    Pothos::Callable(&splitComplexFactory));

/*
 * |PothosDoc Polar to Complex (GPU)
 *
 * Takes in a polar magnitude (port <b>"mag"</b>) and phase (port <b>"phase"</b>) and converts
 * them to a complex number.
 *
 * |category /GPU/Convert
 * |keywords arith complex real imag imaginary magnitude phase rho theta
 * |factory /gpu/arith/polar_to_complex(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The block data type. The output type will be the complex form of this type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerPolarToComplex(
    "/gpu/arith/polar_to_complex",
    Pothos::Callable(&polarToComplexFactory));

/*
 * |PothosDoc Complex to Polar (GPU)
 *
 * Calls <b>af::abs</b> and <b>af::arg</b> on all inputs and outputs results
 * in "mag" and "phase" output channels.
 *
 * |category /GPU/Convert
 * |keywords arith complex real imag imaginary magnitude phase rho theta
 * |factory /gpu/arith/complex_to_polar(device,dtype)
 *
 * |param device[Device] Device to use for processing.
 * |default "Auto"
 *
 * |param dtype[Data Type] The block data type. The input type will be the complex form of this type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 */
static Pothos::BlockRegistry registerComplexToPolar(
    "/gpu/arith/complex_to_polar",
    Pothos::Callable(&complexToPolarFactory));
