// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayFireBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/File.h>
#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include <arrayfire.h>

#include <cassert>
#include <string>
#include <typeinfo>

class FileSourceBlock: public ArrayFireBlock
{
    public:
        static Pothos::Block* make(
            const std::string& filepath,
            const std::string& key,
            bool repeat)
        {
            return new FileSourceBlock(filepath, key, repeat);
        }

        FileSourceBlock(
            const std::string& filepath,
            const std::string& key,
            bool repeat
        ):
            ArrayFireBlock(),
            _filepath(filepath),
            _key(key),
            _repeat(repeat),
            _hasPosted(false),
            _fileContents(),
            _numDims(0)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(FileSourceBlock, getFilepath));
            this->registerCall(this, POTHOS_FCN_TUPLE(FileSourceBlock, getKey));
            this->registerCall(this, POTHOS_FCN_TUPLE(FileSourceBlock, getRepeat));
            this->registerCall(this, POTHOS_FCN_TUPLE(FileSourceBlock, setRepeat));

            const Poco::File pocoFile(_filepath);
            if(!pocoFile.exists())
            {
                throw Pothos::FileNotFoundException(filepath);
            }

            if(-1 == af::readArrayCheck(_filepath.c_str(), _key.c_str()))
            {
                throw Pothos::InvalidArgumentException(
                          "Could not find key in ArrayFire binary",
                          _key);
            }

            auto fileContents = af::readArray(_filepath.c_str(), _key.c_str());
            _numDims = fileContents.numdims();
            if((1 != _numDims) && (2 != _numDims))
            {
                throw Pothos::InvalidArgumentException(
                          "Only arrays of 1-2 dimensions are supported.");
            }

            // Now that we know the file is valid, store the array and
            // initialize our ports.
            _fileContents = std::move(fileContents);
            const auto dtype = Pothos::Object(_fileContents.type()).convert<Pothos::DType>();

            if(1 == _numDims)
            {
                this->setupOutput(0, dtype);
            }
            else
            {
                const size_t nchans = static_cast<size_t>(_fileContents.dims(0));
                for(size_t chan = 0; chan < nchans; ++chan)
                {
                    this->setupOutput(chan, dtype);
                }
            }
        }

        std::string getFilepath() const
        {
            return _filepath;
        };

        std::string getKey() const
        {
            return _key;
        }

        bool getRepeat() const
        {
            return _repeat;
        }

        void setRepeat(bool repeat)
        {
            _repeat = repeat;
        }

        void work()
        {
            if(!_repeat && _hasPosted)
            {
                return;
            }

            if(1 == _numDims)
            {
                this->postAfArray(0, _fileContents);
            }
            else
            {
                this->post2DAfArrayToNumberedOutputPorts(_fileContents);
            }

            _hasPosted = true;
        }

    private:
        std::string _filepath;
        std::string _key;
        bool _repeat;
        bool _hasPosted;

        af::array _fileContents;
        size_t _numDims;
};

static Pothos::BlockRegistry registerFileSource(
    "/arrayfire/stream/file_source",
    Pothos::Callable(&FileSourceBlock::make));
