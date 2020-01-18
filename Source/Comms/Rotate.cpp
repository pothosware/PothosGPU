// Copyright (c) 2014-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-Clause-3

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Util/QFormat.hpp>
#include <iostream>
#include <complex>
#include <cmath>
#include <algorithm> //min/max

using Pothos::Util::floatToQ;

// TODO: parallel

/***********************************************************************
 * |PothosDoc Rotate
 *
 * Perform a complex phase rotation operation on every input element.
 *
 * out[n] = in[n] * exp(j*phase)
 *
 * |category /Math
 * |keywords math phase multiply
 *
 * |param dtype[Data Type] The data type used in the arithmetic.
 * |widget DTypeChooser(cint=1, cfloat=1,dim=1)
 * |default "complex_float32"
 * |preview disable
 *
 * |param phase[Phase] The phase rotation in radians.
 * |units radians
 * |default 0.0
 *
 * |param labelId[Label ID] A optional label ID that can be used to change the phase rotator.
 * Upstream blocks can pass a configurable phase rotator along with the stream data.
 * The rotate block searches input labels for an ID match and interprets the label data as the new phase rotator.
 * |preview valid
 * |default ""
 * |widget StringEntry()
 * |tab Labels
 *
 * |factory /arrayfire/comms/rotate(dtype)
 * |setter setPhase(phase)
 * |setter setLabelId(labelId)
 **********************************************************************/
template <typename Type, typename QType>
class Rotate : public ArrayFireBlock
{
public:
    using AFType = typename PothosToAF<Type>::type;
    using AFQType = typename PothosToAF<QType>::type;

    Rotate(const std::string& device, const size_t dimension):
        ArrayFireBlock(device),
        _phase(0.0),
        _afDType(Pothos::Object(Pothos::DType(typeid(Type))).convert<af::dtype>()),
        _afQDType(Pothos::Object(Pothos::DType(typeid(QType))).convert<af::dtype>())
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(Rotate, setPhase));
        this->registerCall(this, POTHOS_FCN_TUPLE(Rotate, getPhase));
        this->registerCall(this, POTHOS_FCN_TUPLE(Rotate, setLabelId));
        this->registerCall(this, POTHOS_FCN_TUPLE(Rotate, getLabelId));
        this->setupInput(0, Pothos::DType(typeid(Type), dimension));
        this->setupOutput(0, Pothos::DType(typeid(Type), dimension));
    }

    void setPhase(const double phase)
    {
        _phase = phase;
        _phasor = PothosToAF<QType>::to(floatToQ<QType>(std::polar(1.0, phase)));
    }

    double getPhase(void) const
    {
        return _phase;
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
        auto elems = this->workInfo().minElements;
        if (elems == 0) return;

        auto inPort = this->input(0);
        auto afArray = this->getInputPortAsAfArray(0);

        // Check the labels for rotation phase
        if (not _labelId.empty()) for (const auto &label : inPort->labels())
        {
            if (label.index >= elems) break; // Ignore labels past input bounds
            if (label.id == _labelId)
            {
                // Only set scale-phase when the label is at the front
                if (label.index == 0)
                {
                    this->setPhase(label.data.template convert<double>());
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
         *     const QType tmp = _phasor*QType(in[i]);
         *     out[i] = fromQ<Type>(tmp);
         * }
         * ---------------------------------------------------
         */
        afArray = afArray.as(_afQDType);
        afArray *= _phasor;
        afArray = afArray.as(_afDType);

        inPort->consume(elems);
        this->postAfArray(0, std::move(afArray));
    }

private:
    double _phase;
    AFQType _phasor;
    std::string _labelId;

    af::dtype _afDType;
    af::dtype _afQDType;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *rotateFactory(
    const std::string& device,
    const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory_(type, qtype) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new Rotate<type, qtype>(device, dtype.dimension());
    #define ifTypeDeclareFactory(type, qtype) \
        ifTypeDeclareFactory_(std::complex<type>, std::complex<qtype>)
    ifTypeDeclareFactory(double, double);
    ifTypeDeclareFactory(float, float);
    throw Pothos::InvalidArgumentException("rotateFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerRotate(
    "/arrayfire/comms/rotate", &rotateFactory);
