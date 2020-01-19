// Copyright (c) 2014-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-Clause-3

#include "ArrayFireBlock.hpp"
#include "Functions.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <complex>

static const size_t defaultWaveTableSize = 4096;
static const size_t maxWaveTableSize = 1024*1024;
static const size_t minimumTableStepSize = 16;

/***********************************************************************
 * |PothosDoc Waveform Source
 *
 * The waveform source produces simple cyclical waveforms.
 * When a complex data type is chosen, the real and imaginary
 * components of the outputs will be 90 degrees out of phase.
 *
 * |category /Sources
 * |category /Waveforms
 * |keywords cosine sine ramp square waveform source signal
 *
 * |param dtype[Data Type] The data type produced by the waveform source.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1)
 * |default "complex_float32"
 * |preview disable
 *
 * |param wave[Wave Type] The type of the waveform produced.
 * |option [Constant] "CONST"
 * |option [Sinusoid] "SINE"
 * |option [Ramp] "RAMP"
 * |option [Square] "SQUARE"
 * |default "SINE"
 *
 * |param rate[Sample Rate] The sample rate of the waveform.
 * |units samples/sec
 * |default 1.0
 *
 * |param freq[Frequency] The frequency of the waveform (+/- 0.5*rate).
 * |units Hz
 * |default 0.1
 *
 * |param ampl[Amplitude] A constant scalar representing the amplitude.
 * |default 1.0
 *
 * |param offset A constant value added to the waveform after scaling.
 * |default 0.0
 * |preview valid
 *
 * |param res[Resolution] The resolution of the internal wave table (0.0 for automatic).
 * When unspecified, the wave table size will be configured for the user's requested frequency.
 * Specify a minimum resolution in Hz to fix the size of the wave table.
 * |units Hz
 * |default 0.0
 * |preview valid
 *
 * |factory /arrayfire/comms/waveform_source(dtype)
 * |setter setSampleRate(rate)
 * |setter setWaveform(wave)
 * |setter setOffset(offset)
 * |setter setAmplitude(ampl)
 * |setter setFrequency(freq)
 * |setter setResolution(res)
 **********************************************************************/
template <typename Type>
class WaveformSource : public ArrayFireBlock
{
public:
    using AFType = typename PothosToAF<Type>::type;

    WaveformSource(const std::string& device):
        ArrayFireBlock(device),
        _rate(1.0),
        _freq(0.0),
        _res(0.0),
        _offset(PothosToAF<std::complex<double>>::to(0.0)),
        _scalar(PothosToAF<std::complex<double>>::to(1.0)),
        _afTable(),
        _afDType(Pothos::Object(Pothos::DType(typeid(Type))).convert<af::dtype>()),
        _wave("CONST")
    {
        this->setupOutput(0, typeid(Type));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, setWaveform));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, getWaveform));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, setOffset));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, getOffset));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, setAmplitude));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, getAmplitude));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, setFrequency));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, getFrequency));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, setSampleRate));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, getSampleRate));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, setResolution));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveformSource<Type>, getResolution));
    }

    void activate(void)
    {
        this->updateTable();
    }

    void work(void)
    {
        // Since we post buffers, instead of pulling out values from the table
        // to match the output size, simply post the whole thing. This solves
        // the issue of having to query and set specific indices, which has to
        // either be done on the host or in an inefficient GPU operation. We
        // may need to change this if the larger buffers become unwieldy.
        this->postAfArray(0, _afTable);
    }

    void setWaveform(const std::string &wave)
    {
        _wave = wave;
        this->updateTable();
    }

    std::string getWaveform(void)
    {
        return _wave;
    }

    void setOffset(const std::complex<double> &offset)
    {
        _offset = PothosToAF<std::complex<double>>::to(offset);
        this->updateTable();
    }

    std::complex<double> getOffset(void)
    {
        return PothosToAF<std::complex<double>>::from(_offset);
    }

    void setAmplitude(const std::complex<double> &scalar)
    {
        _scalar = PothosToAF<std::complex<double>>::to(scalar);
        this->updateTable();
    }

    std::complex<double> getAmplitude(void)
    {
        return PothosToAF<std::complex<double>>::from(_scalar);
    }

    void setFrequency(const double &freq)
    {
        _freq = freq;
        this->updateTable();
    }

    double getFrequency(void)
    {
        return _freq;
    }

    void setSampleRate(const double &rate)
    {
        _rate = rate;
        this->updateTable();
    }

    double getSampleRate(void)
    {
        return _rate;
    }

    void setResolution(const double &res)
    {
        _res = res;
        this->updateTable();
    }

    double getResolution(void)
    {
        return _res;
    }

private:

    void updateTable(void)
    {
        if (not this->isActive()) return;

        //This fraction (of a period) is used to determine table size efficacy.
        //When specified, use the resolution, otherwise the user's frequency.
        const auto frac = ((_res == 0.0)?_freq:_res)/_rate;

        //loop for a table size that meets the minimum step
        size_t numEntries = defaultWaveTableSize;
        while (true)
        {
            const auto delta = std::llround(frac*numEntries);
            if (frac == 0.0) break;
            if (size_t(std::abs(delta)) >= minimumTableStepSize) break;
            if (numEntries*2 > maxWaveTableSize) break;
            numEntries *= 2;
        }

        if (_wave == "CONST")
        {
            /*
             * for (size_t i = 0; i < _table.size(); i++)
             * {
             *     this->setElem(_table[i], 1.0);
             * }
             */
            _afTable = af::range(static_cast<dim_t>(numEntries)).as(_afDType);
        }
        else if (_wave == "SINE")
        {
            /*
             * for (size_t i = 0; i < _table.size(); i++){
             *     this->setElem(_table[i], std::polar(1.0, 2*M_PI*i/_table.size()));
             * }
             */
            auto rho = af::identity(static_cast<dim_t>(numEntries)).as(::f64);

            auto theta = af::range(static_cast<dim_t>(numEntries)).as(::f64);
            theta *= 2.0;
            theta *= af::Pi;
            theta /= static_cast<double>(numEntries);

            _afTable = polarToComplex(rho, theta);
        }
        else if (_wave == "RAMP")
        {
            /*
             * for (size_t i = 0; i < _table.size(); i++)
             * {
             *     const size_t q = (i+(3*_table.size())/4)%_table.size();
             *     this->setElem(_table[i], std::complex<double>(
             *         2.0*i/(_table.size()-1) - 1.0,
             *         2.0*q/(_table.size()-1) - 1.0
             *     ));
             * }
             */
            _afTable = af::complex(
                           (I(numEntries) * 2.0 / static_cast<double>(numEntries-1)) - 1.0,
                           (Q(numEntries) * 2.0 / static_cast<double>(numEntries-1)) - 1.0);
        }
        else if (_wave == "SQUARE")
        {
            /*
             * for (size_t i = 0; i < _table.size(); i++)
             * {
             *     const size_t q = (i+(3*_table.size())/4)%_table.size();
             *     this->setElem(_table[i], std::complex<double>(
             *         (i < _table.size()/2)? 0.0 : 1.0,
             *         (q < _table.size()/2)? 0.0 : 1.0
             *     ));
             * }
             */

            // Note: comparison operators return an int8 array of 0 or 1.
            _afTable = af::complex(
                           (I(numEntries) >= (numEntries/2)).as(::f64),
                           (Q(numEntries) >= (numEntries/2)).as(::f64));
        }
        else throw Pothos::InvalidArgumentException("WaveformSource::setWaveform("+_wave+")", "unknown waveform setting");

        _afTable = applyScalarAndOffset(_afTable);
    }

    af::array I(size_t tableSize)
    {
        return af::identity(static_cast<dim_t>(tableSize)).as(::f64);
    }

    af::array Q(size_t tableSize)
    {
        /*
         * for (size_t i = 0; i < _table.size(); i++)
         * {
         *     const size_t q = (i+(3*_table.size())/4)%_table.size();
         * }
         */
        return ((I(tableSize) + (3 * tableSize) / 4) % tableSize).as(::f64);
    }

    af::array applyScalarAndOffset(const af::array& afArray)
    {
        af::array ret = ((afArray * _scalar) + _offset);
        if(afArray.iscomplex()) ret = af::real(ret);

        return ret.as(_afDType);
    }

    PothosToAF<double>::type _rate, _freq, _res;
    PothosToAF<std::complex<double>>::type _offset, _scalar;
    af::array _afTable;
    af::dtype _afDType;

    std::string _wave;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *waveformSourceFactory(
    const std::string &device,
    const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory(type) \
        if (dtype == Pothos::DType(typeid(type))) return new WaveformSource<type>(device); \
        if (dtype == Pothos::DType(typeid(std::complex<type>))) return new WaveformSource<std::complex<type>>(device);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(double);
    throw Pothos::InvalidArgumentException("waveformSourceFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerWaveformSource(
    "/arrayfire/comms/waveform_source", &waveformSourceFactory);
