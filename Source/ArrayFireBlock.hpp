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

        std::string getArrayFireBackend() const;

        void setArrayFireBackend(const std::string& backend);

        bool getBlockAssumesArrayFireInputs() const;

        void setBlockAssumesArrayFireInputs(bool value);

        //
        // Input port API
        //

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
        // Member variables
        //

        bool _assumeArrayFireInputs;

        af::Backend _afBackend;

    private:

        template <typename PortIdType>
        af::array _getInputPortAsAfArray(
            const PortIdType& portId,
            bool truncateToMinLength);

        template <typename PortIdType, typename AfArrayType>
        void _postAfArray(
            const PortIdType& portId,
            const AfArrayType& afArray);
};
