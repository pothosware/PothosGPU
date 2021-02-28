// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#include <string>

namespace PothosGPU
{

class ArrayFireBlock: public Pothos::Block
{
    public:
        ArrayFireBlock() = delete;
        explicit ArrayFireBlock(const std::string& device);

        virtual ~ArrayFireBlock();

    protected:

        Pothos::BufferManager::Sptr getInputBufferManager(
            const std::string& name,
            const std::string& domain) override;

        Pothos::BufferManager::Sptr getOutputBufferManager(
            const std::string& name,
            const std::string& domain) override;

        void activate() override;

        std::string backend() const;

        std::string device() const;

        virtual std::string overlay() const;

        //
        // Input port API
        //

        af::array getInputPortAsAfArray(
            size_t portNum,
            bool truncateToMinLength = true);

        af::array getInputPortAsAfArray(
            const std::string& portName,
            bool truncateToMinLength = true);

        //
        // Output port API
        //

        void produceFromAfArray(
            size_t portNum,
            const af::array& afArray);

        void produceFromAfArray(
            const std::string& portName,
            const af::array& afArray);

        void postAfArray(
            size_t portNum,
            const af::array& afArray);

        void postAfArray(
            const std::string& portName,
            const af::array& afArray);

        //
        // Misc
        //

        void configArrayFire() const;

        //
        // Member variables
        //

        af::Backend _afBackend;
        int _afDevice;
        std::string _afDeviceName;
        std::string _domain;

    private:

        template <typename PortIdType>
        af::array _getInputPortAsAfArray(
            const PortIdType& portId,
            bool truncateToMinLength);

        template <typename PortIdType, typename AfArrayType>
        void _produceFromAfArray(
            const PortIdType& portId,
            const AfArrayType& afArray);

        template <typename PortIdType, typename AfArrayType>
        void _postAfArray(
            const PortIdType& portId,
            const AfArrayType& afArray);
};

}
