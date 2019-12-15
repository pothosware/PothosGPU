// Copyright (c) 2019 Nicholas Corgan
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
            ArrayFireBlock(),
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

            auto afDType = Pothos::Object(this->input(0)->dtype()).convert<af::dtype>();
            auto maxElementsIter = std::max_element(
                                       _buffers.begin(),
                                       _buffers.end(),
                                       bufElemCmp);
            const auto maxElements = maxElementsIter->elements();

            af::array afArray(
                          static_cast<dim_t>(_nchans),
                          static_cast<dim_t>(maxElements),
                          afDType);
            for(size_t chan = 0; chan < _nchans; ++chan)
            {
                af::array afArray(maxElements, 0);
                afArray.write(
                    _buffers[chan].as<const std::uint8_t*>(),
                    _buffers[chan].length);
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

static Pothos::BlockRegistry registerFileSink(
    "/arrayfire/array/file_sink",
    Pothos::Callable(&FileSinkBlock::make));
