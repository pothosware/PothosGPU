#include "OneToOneBlock.hpp"
#include "NToOneBlock.hpp"
#include "TwoToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Proxy.hpp>

#include <arrayfire.h>

#include <vector>

namespace PothosGPU
{

static const std::vector<Pothos::BlockRegistry> BlockRegistries =
{
%for block in oneToOneBlocks:
    Pothos::BlockRegistry(
        "/gpu/${block["header"]}/${block["blockName"]}",
    %if block.get("pattern", "") == "FloatToComplex":
        Pothos::Callable(&OneToOneBlock::makeFloatToComplex)
            .bind<OneToOneFunc>(&af::${block["func"]}, 1)
    %elif block.get("pattern", "") == "ComplexToFloat":
        Pothos::Callable(&OneToOneBlock::makeComplexToFloat)
            .bind<OneToOneFunc>(&af::${block["func"]}, 1)
    %else:
        Pothos::Callable(&OneToOneBlock::makeFromOneType)
            .bind<OneToOneFunc>(&af::${block["func"]}, 1)
            .bind<DTypeSupport>({
                ${"true" if block["supportedTypes"].get("supportInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportUInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportComplexFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
            }, 3)
    %endif
    ),
%endfor
%for block in twoToOneBlocks:
    Pothos::BlockRegistry(
        "/gpu/${block["header"]}/${block["blockName"]}",
    %if block.get("pattern", "") == "FloatToComplex":
        Pothos::Callable(&TwoToOneBlock::makeFloatToComplex)
            .bind<TwoToOneFunc>(&af::${block["func"]}, 1)
            .bind<bool>(${"true" if block.get("allowZeroInBuffer1", True) else "false"}, 3)
    %else:
        Pothos::Callable(&TwoToOneBlock::makeFromOneType)
            .bind<TwoToOneFunc>(&af::${block["blockName"]}, 1)
            .bind<DTypeSupport>({
                ${"true" if block["supportedTypes"].get("supportInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportUInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportComplexFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
            }, 3)
            .bind<bool>(${"true" if block.get("allowZeroInBuffer1", True) else "false"}, 4)
    %endif
    ),
%endfor
%for block in NToOneBlocks:
    Pothos::BlockRegistry(
        "/gpu/${block["header"]}/${block["blockName"]}",
        Pothos::Callable(&NToOneBlock::make)
    %if "operator" in block:
            .bind<NToOneFunc>(AF_ARRAY_OP_N_TO_ONE_FUNC(${block["operator"]}), 1)
    %else:
            .bind<NToOneFunc>(&af::${block["func"]}, 1)
    %endif
            .bind<DTypeSupport>({
                ${"true" if block["supportedTypes"].get("supportInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportUInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportComplexFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
            }, 4)
            .bind<bool>(${"true" if block.get("postBuffer", True) else "false"}, 5)
    ),
%endfor
};

pothos_static_block(register_pothos_gpu_docs)
{
%for doc in docs:
    ${doc}
%endfor
}

}
