// Copyright (c) 2014-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "BufferConversions.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Util/QFormat.hpp>

#include <cstdint>
#include <iostream>
#include <complex>
#include <algorithm> //min/max

using Pothos::Util::floatToQ;

// TODO: parallel

/***********************************************************************
 * |PothosDoc Scale
 *
 * Perform a multiply by scalar operation on every input element.
 *
 * out[n] = in[n] * factor
 *
 * |category /Math
 * |keywords math scale multiply factor gain
 *
 * |param dtype[Data Type] The data type used in the arithmetic.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,dim=1)
 * |default "complex_float32"
 * |preview disable
 *
 * |param factor[Factor] The multiplication scale factor.
 * |default 0.0
 *
 * |param labelId[Label ID] A optional label ID that can be used to change the scale factor.
 * Upstream blocks can pass a configurable scale factor along with the stream data.
 * The scale block searches input labels for an ID match and interprets the label data as the new scale factor.
 * |preview valid
 * |default ""
 * |widget StringEntry()
 * |tab Labels
 *
 * |factory /arrayfire/comms/scale(dtype)
 * |setter setFactor(factor)
 * |setter setLabelId(labelId)
 **********************************************************************/
template <typename Type, typename QType, typename ScaleType>
class Scale : public ArrayFireBlock
{
public:
    using AFType = typename PothosToAF<Type>::type;
    using AFQType = typename PothosToAF<QType>::type;
    using AFScaleType = typename PothosToAF<ScaleType>::type;

    Scale(const std::string& device, const size_t dimension):
        ArrayFireBlock(device),
        _factor(0.0),
        _afDType(Pothos::Object(Pothos::DType(typeid(Type))).convert<af::dtype>()),
        _afQDType(Pothos::Object(Pothos::DType(typeid(QType))).convert<af::dtype>()),
        _afScaleDType(Pothos::Object(Pothos::DType(typeid(ScaleType))).convert<af::dtype>())
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(Scale, setFactor));
        this->registerCall(this, POTHOS_FCN_TUPLE(Scale, getFactor));
        this->registerCall(this, POTHOS_FCN_TUPLE(Scale, setLabelId));
        this->registerCall(this, POTHOS_FCN_TUPLE(Scale, getLabelId));

        this->setupInput(0, Pothos::DType(typeid(Type), dimension));
        this->setupOutput(
            0,
            Pothos::DType(typeid(Type), dimension),
            this->getPortDomain());
    }

    void setFactor(const double factor)
    {
        _factor = factor;
        _factorScaled = PothosToAF<ScaleType>::to(floatToQ<ScaleType>(_factor));
    }

    double getFactor(void) const
    {
        return _factor;
    }

    void setLabelId(const std::string &id)
    {
        _labelId = id;
    }

    std::string getLabelId(void) const
    {
        return _labelId;
    }

    void work(void)
    {
        // Number of elements to work with
        auto elems = this->workInfo().minInElements;
        if (elems == 0) return;

        auto inPort = this->input(0);
        auto afArray = this->getInputPortAsAfArray(0);

        // Check the labels for scale factors
        if (not _labelId.empty()) for (const auto &label : inPort->labels())
        {
            if (label.index >= elems) break; // Ignore labels past input bounds
            if (label.id == _labelId)
            {
                // Only set scale-factor when the label is at the front
                if (label.index == 0)
                {
                    this->setFactor(label.data.template convert<double>());
                }
                // Otherwise stop processing before the next label
                // on the next call, this label will be index 0
                else
                {
                    elems = label.index;
                    break;
                }
            }
        }

        /*
         * Perform scale operation. ArrayFire vectorizes theses operations,
         * optimizing it from the original implementation.
         *
         * Original:
         * ---------------------------------------------------
         * const size_t N = elems*inPort->dtype().dimension();
         * for (size_t i = 0; i < N; i++)
         * {
         *     const QType tmp = _factorScaled*QType(in[i]);
         *     out[i] = fromQ<Type>(tmp);
         * }
         * ---------------------------------------------------
         */
        afArray = afArray.as(_afQDType);
        afArray *= _factorScaled;
        afArray = afArray.as(_afDType);

        this->postAfArray(0, std::move(afArray));
    }

private:
    double _factor;
    AFScaleType _factorScaled;
    std::string _labelId;

    af::dtype _afDType;
    af::dtype _afQDType;
    af::dtype _afScaleDType;
};

/***********************************************************************
 * Registration
 **********************************************************************/
static Pothos::Block *scaleFactory(
    const std::string& device,
    const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory_(type, qtype, scaleType) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new Scale<type, qtype, scaleType>(device, dtype.dimension());
    #define ifTypeDeclareFactory(type, qtype) \
        ifTypeDeclareFactory_(type, qtype, qtype) \
        ifTypeDeclareFactory_(std::complex<type>, std::complex<qtype>, qtype)
    ifTypeDeclareFactory(float, float);
    ifTypeDeclareFactory(double, double);
    throw Pothos::InvalidArgumentException("scaleFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerScale(
    "/arrayfire/comms/scale", &scaleFactory);
