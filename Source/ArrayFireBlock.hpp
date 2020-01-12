// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Pothos/Framework.hpp>

#include <arrayfire.h>

#include <string>

class ArrayFireBlock: public Pothos::Block
{
    public:
        ArrayFireBlock();
        virtual ~ArrayFireBlock();

    protected:

        // TODO: single function that deals with both

        std::string getArrayFireBackend() const;

        void setArrayFireBackend(const Pothos::Object& backend);

        std::string getArrayFireDevice() const;

        void setArrayFireDevice(const std::string& device);

        std::string getPortDomain() const;

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
};
