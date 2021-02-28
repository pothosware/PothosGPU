// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "DeviceCache.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/File.h>
#include <Poco/Logger.h>
#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Path.h>

#include <arrayfire.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace PothosGPU;

// To avoid collisions
namespace
{

class FileSinkBlock: public ArrayFireBlock
{
    public:
        static Pothos::Block* make(
            const std::string& filepath,
            const std::string& key,
            const Pothos::DType& dtype,
            size_t numChannels,
            bool append)
        {
            return new FileSinkBlock(filepath, key, dtype, numChannels, append);
        }

        FileSinkBlock(
            const std::string& filepath,
            const std::string& key,
            const Pothos::DType& dtype,
            size_t numChannels,
            bool append
        ):
            ArrayFireBlock(getCPUOrBestDevice()),
            _filepath(filepath),
            _key(key),
            _append(append),
            _nchans(numChannels)
        {
            const Poco::File pocoFile(_filepath);
            if(pocoFile.exists())
            {
                // Make sure the file is writable before writing to it.
                if(!pocoFile.isFile())
                {
                    throw Pothos::FileException(
                            "This path is valid but does not correspond to a regular file.",
                            _filepath);
                }
                if(!pocoFile.canWrite())
                {
                    throw Pothos::FileReadOnlyException(_filepath);
                }

                // Make sure this is an ArrayFire binary.
                try
                {
                    af::readArray(_filepath.c_str(), 0U);
                }
                catch(...)
                {
                    throw Pothos::DataFormatException(
                              "This file exists but is not a valid ArrayFire binary.",
                              _filepath);
                }

                // If the file already contains an array with the given key,
                // and we want to append to it, we need to adhere to this
                // type.
                if(_append && (-1 != af::readArrayCheck(_filepath.c_str(), _key.c_str())))
                {
                    auto arr = af::readArray(_filepath.c_str(), _key.c_str());
                    const auto numDims = arr.numdims();
                    if((1 != numDims) && (2 != numDims))
                    {
                        throw Pothos::DataFormatException(
                                  "Only arrays of 1-2 dimensions are supported.");
                    }

                    auto arrNChans = static_cast<size_t>(arr.dims().dims[0]);
                    auto arrDType = Pothos::Object(arr.type()).convert<Pothos::DType>();

                    if(!isSupportedFileSinkType(arrDType))
                    {
                        throw Pothos::DataFormatException(
                                  Poco::format(
                                      "Cannot append to array \"%s\", as it is of unsupported type \"%s\".",
                                      key,
                                      arrDType.name()));
                    }
                    if((arrNChans != _nchans) || (arrDType != dtype))
                    {
                        throw Pothos::DataFormatException(
                                  Poco::format(
                                      "Cannot append to the existing array (%s, %s chans)",
                                      arrDType.name(),
                                      Poco::NumberFormatter::format(arrNChans)),
                                  Poco::format(
                                      "Input: %s, %s chans",
                                      dtype.name(),
                                      Poco::NumberFormatter::format(_nchans)));
                    }
                }
            }
            else
            {
                // There's no file, so just make sure we can write to the
                // directory.
                auto parentDirFile = Poco::File(Poco::Path(pocoFile.path()).parent());
                if(!parentDirFile.canWrite())
                {
                    throw Pothos::FileAccessDeniedException(
                              "Cannot write a file to the parent directory",
                              _filepath);
                }
            }

            for(size_t chan = 0; chan < _nchans; ++chan)
            {
                this->setupInput(chan, dtype, _domain);
            }

            _buffers.resize(_nchans);

            this->registerCall(this, POTHOS_FCN_TUPLE(FileSinkBlock, filepath));
            this->registerCall(this, POTHOS_FCN_TUPLE(FileSinkBlock, key));
            this->registerCall(this, POTHOS_FCN_TUPLE(FileSinkBlock, append));
        }

        void deactivate()
        {
            this->configArrayFire();

            static const auto bufElemCmp =
                [](const Pothos::BufferChunk& buf1, const Pothos::BufferChunk& buf2)
                {
                    return (buf1.elements() < buf2.elements());
                };

            af::array afArray;
            if(1 == _nchans)
            {
                afArray = Pothos::Object(_buffers[0]).convert<af::array>();
            }
            else
            {
                auto afDType = Pothos::Object(this->input(0)->dtype()).convert<af::dtype>();
                auto maxElementsIter = std::max_element(
                                           _buffers.begin(),
                                           _buffers.end(),
                                           bufElemCmp);
                const auto maxElements = maxElementsIter->elements();

                afArray = af::array(
                              static_cast<dim_t>(_nchans),
                              static_cast<dim_t>(maxElements),
                              afDType);
                for(size_t chan = 0; chan < _nchans; ++chan)
                {
                    afArray.row(chan) = Pothos::Object(_buffers[chan]).convert<af::array>();
                }
            }

            af::saveArray(
                _key.c_str(),
                afArray,
                _filepath.c_str(),
                _append);
        }

        std::string filepath() const
        {
            return _filepath;
        };

        std::string key() const
        {
            return _key;
        }

        bool append() const
        {
            return _append;
        }

        void work()
        {
            const auto elems = this->workInfo().minInElements;
            if(0 == elems)
            {
                return;
            }

            // Just accumulate the buffers we're given.
            for(size_t chan = 0; chan < _nchans; ++chan)
            {
                auto* inputPort = this->input(chan);
                if(inputPort->elements() > 0)
                {
                    const auto& buffer = inputPort->buffer();

                    inputPort->consume(buffer.elements());
                    _buffers[chan].append(buffer);
                }
            }
        }

    private:
        std::string _filepath;
        std::string _key;
        bool _append;
        size_t _nchans;

        std::vector<Pothos::BufferChunk> _buffers;
};

/*
 * |PothosDoc ArrayFire File Sink
 *
 * Calls <b>af::writeArray</b> to write an array to an ArrayFire binary file.
 * These binary files can store multiple arrays, so a key parameter is given
 * to select a specific array. This block supports:
 * <ol>
 * <li>Creating a new file</li>
 * <li>Adding an array to an existing file</li>
 * <li>Replacing an array in an existing file</li>
 * <li>Appending to an array in an existing file</li>
 * </ol>
 *
 * <b>NOTE:</b> Unlike other ArrayFire blocks, this block does not support the
 * following types, due to an ArrayFire bug that doesn't preserve the values
 * passed into <b>af::writeArray</b>.
 * <ul>
 * <li><b>int32</b></li>
 * <li><b>int64</b></li>
 * <li><b>uint32</b></li>
 * <li><b>uint64</b></li>
 * </ul>
 *
 * |category /GPU/File IO
 * |category /File IO
 * |category /Sinks
 * |keywords array file sink io
 * |factory /gpu/array/file_sink(filepath,key,dtype,numChannels,append)
 *
 * |param filepath[Filepath] The path of the ArrayFire binary file.
 * |widget FileEntry(mode=save)
 * |default ""
 * |preview enable
 *
 * |param key[Key] The key of the array stored in the ArrayFire binary file.
 * |widget StringEntry()
 * |default "key"
 * |preview enable
 *
 * |param dtype[Data Type] The output's data type.
 * If appending to an existing array, this value must match the type of the
 * existing array.
 * |widget DTypeChooser(int=1,uint=1,float=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param numChannels[Num Channels] The number of channels.
 * If appending to an existing array, this value must match the dimensions of
 * the existing array.
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview disable
 *
 * |param append[Append?]
 * |default false
 * |widget ToggleSwitch(on="True",off="False")
 * |preview enable
 */
static Pothos::BlockRegistry registerFileSink(
    "/gpu/array/file_sink",
    Pothos::Callable(&FileSinkBlock::make));

}
