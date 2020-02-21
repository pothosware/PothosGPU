// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

#include <vector>

class Root: public OneToOneBlock
{
    public:
        static Pothos::Block* make(
            const std::string& device,
            const Pothos::DType& dtype,
            double root)
        {
            // Supports float, complex
            static const DTypeSupport dtypeSupport{false,false,true,true};
            validateDType(dtype, dtypeSupport);

            return new Root(device, dtype, root);
        }

        Root(const std::string& device,
            const Pothos::DType& dtype,
            double root
        ):
            OneToOneBlock(
                device,
                Pothos::Callable(), // Will be set later
                dtype,
                dtype)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Root, getRoot));
            this->registerCall(this, POTHOS_FCN_TUPLE(Root, setRoot));

            this->registerProbe("getRoot");
            this->registerSignal("rootChanged");

            this->setRoot(root);
        }

        virtual ~Root() {};

        double getRoot() const
        {
            return _root;
        }

        void setRoot(double root)
        {
            _root = root;

            if(2.0 == root)
            {
                _func = Pothos::Callable(&af::sqrt);
            }
            else if(3.0 == root)
            {
                _func = Pothos::Callable(&af::cbrt);
            }
            else
            {
                // To disambiguate an overloaded function
                using RootType = af::array(*)(const af::array&, const double);

                _func = Pothos::Callable(RootType(af::root));
                _func.bind(_root, 1);
            }

            this->emitSignal("rootChanged", root);
        }

    private:
        double _root;
};
