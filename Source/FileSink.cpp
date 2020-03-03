// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
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
#include <string>
#include <vector>

class FileSinkBlock: public ArrayFireBlock
{
    public:
        static Pothos::Block* make(
            const std::string& device,
            const std::string& filepath,
            const std::string& key,
            const Pothos::DType& dtype,
            size_t numChannels,
            bool append)
        {
            return new FileSinkBlock(device, filepath, key, dtype, numChannels, append);
        }

        FileSinkBlock(
            const std::string& device,
            const std::string& filepath,
            const std::string& key,
            const Pothos::DType& dtype,
            size_t numChannels,
            bool append
        ):
            ArrayFireBlock(device),
            _filepath(filepath),
            _key(key),
            _append(append),
            _nchans(numChannels)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(FileSinkBlock, getFilepath));
            this->registerCall(this, POTHOS_FCN_TUPLE(FileSinkBlock, getKey));
            this->registerCall(this, POTHOS_FCN_TUPLE(FileSinkBlock, getAppend));

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

                    // Note, 2019/12/10:
                    // Pothos::DType doesn't currently have a != operator.
                    if((arrNChans != _nchans) || !(arrDType == dtype))
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
                this->setupInput(chan, dtype);
            }

            _buffers.resize(_nchans);
        }

        void deactivate()
        {
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

        std::string getFilepath() const
        {
            return _filepath;
        };

        std::string getKey() const
        {
            return _key;
        }

        bool getAppend() const
        {
            return _append;
        }

        void work()
        {
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

// TODO: setKey initializer
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
 * |category /ArrayFire/File IO
 * |keywords array file sink io
 * |factory /arrayfire/array/file_sink(device,filepath,key,dtype,numChannels,append)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
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
 * |widget DTypeChooser(int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1,dim=1)
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
    "/arrayfire/array/file_sink",
    Pothos::Callable(&FileSinkBlock::make));
