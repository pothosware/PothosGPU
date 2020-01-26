// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#include <string>

class ArrayFireBlock: public Pothos::Block
{
    public:
        ArrayFireBlock(const std::string& device);

        virtual ~ArrayFireBlock();

    protected:

        std::shared_ptr<Pothos::BufferManager> getInputBufferManager(
            const std::string& name,
            const std::string& domain);

        std::shared_ptr<Pothos::BufferManager> getOutputBufferManager(
            const std::string& name,
            const std::string& domain);

        std::string getArrayFireBackend() const;

        std::string getArrayFireDevice() const;

        std::string getPortDomain() const;

        virtual std::string overlay() const;

        //
        // Input port API
        //

        bool doesInputPortDomainMatch(size_t portNum) const;

        bool doesInputPortDomainMatch(const std::string& portName) const;

        const af::array& getInputPortAfArrayRef(size_t portNum);

        const af::array& getInputPortAfArrayRef(const std::string& portName);

        af::array getInputPortAsAfArray(
            size_t portNum,
            bool truncateToMinLength = true);

        af::array getInputPortAsAfArray(
            const std::string& portName,
            bool truncateToMinLength = true);

        af::array getNumberedInputPortsAs2DAfArray();

        //
        // Output port API
        //

        void postAfArray(
            size_t portNum,
            const af::array& afArray);

        void postAfArray(
            const std::string& portName,
            const af::array& afArray);

        void postAfArray(
            size_t portNum,
            af::array&& rAfArray);

        void postAfArray(
            const std::string& portName,
            af::array&& rAfArray);

        void post2DAfArrayToNumberedOutputPorts(const af::array& afArray);

        //
        // Debug
        //

        void debugLogInputPortElements();

        //
        // Member variables
        //

        af::Backend _afBackend;
        int _afDevice;
        std::string _afDeviceName;

    private:

        template <typename PortIdType>
        bool _doesInputPortDomainMatch(const PortIdType& portId) const;

        template <typename PortIdType>
        const af::array& _getInputPortAfArrayRef(const PortIdType& portId);

        template <typename PortIdType>
        af::array _getInputPortAsAfArray(
            const PortIdType& portId,
            bool truncateToMinLength);

        template <typename PortIdType, typename AfArrayType>
        void _postAfArray(
            const PortIdType& portId,
            const AfArrayType& afArray);

        template <typename PortIdType, typename AfArrayType>
        void _postAfArray(
            const PortIdType& portId,
            AfArrayType&& rAfArray);
};
