#include "OneToOneBlock.hpp"
#include "SingleOutputSource.hpp"
#include "TwoToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <arrayfire.h>

#include <vector>

static const std::vector<Pothos::BlockRegistry> BlockRegistries =
{
%for block in oneToOneBlocks:
    Pothos::BlockRegistry(
        "/arrayfire/${block["header"]}/${block["func"]}",
    %if "supportedInputTypes" in block:
        Pothos::Callable(&OneToOneBlock::makeFromTwoTypes)
            .bind<OneToOneFunc>(&af::${block["func"]}, 0)
            .bind<DTypeSupport>({
                ${"true" if block["supportedInputTypes"].get("supportInt", block["supportedInputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedInputTypes"].get("supportUInt", block["supportedInputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedInputTypes"].get("supportFloat", block["supportedInputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedInputTypes"].get("supportComplexFloat", block["supportedInputTypes"].get("supportAll", False)) else "false"},
            }, 3)
            .bind<DTypeSupport>({
                ${"true" if block["supportedOutputTypes"].get("supportInt", block["supportedOutputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedOutputTypes"].get("supportUInt", block["supportedOutputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedOutputTypes"].get("supportFloat", block["supportedOutputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedOutputTypes"].get("supportComplexFloat", block["supportedOutputTypes"].get("supportAll", False)) else "false"},
            }, 4)
    %else:
        Pothos::Callable(&OneToOneBlock::makeFromOneType)
            .bind<OneToOneFunc>(&af::${block["func"]}, 0)
            .bind<DTypeSupport>({
                ${"true" if block["supportedTypes"].get("supportInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportUInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportComplexFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
            }, 2)
    %endif
    ),
%endfor
%for block in singleOutputSources:
    Pothos::BlockRegistry(
        "/arrayfire/${block["header"]}/${block["func"]}",
        Pothos::Callable(&SingleOutputSource::make)
            .bind<SingleOutputFunc>(&af::${block["func"]}, 0)
            .bind<DTypeSupport>({
                ${"true" if block["supportedTypes"].get("supportInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportUInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportComplexFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
            }, 2)
    ),
%endfor
%for block in twoToOneBlocks:
    Pothos::BlockRegistry(
        "/arrayfire/${block["header"]}/${block["func"]}",
    %if "supportedInputTypes" in block:
        Pothos::Callable(&TwoToOneBlock::makeFromTwoTypes)
            .bind<TwoToOneFunc>(&af::${block["func"]}, 0)
            .bind<DTypeSupport>({
                ${"true" if block["supportedInputTypes"].get("supportInt", block["supportedInputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedInputTypes"].get("supportUInt", block["supportedInputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedInputTypes"].get("supportFloat", block["supportedInputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedInputTypes"].get("supportComplexFloat", block["supportedInputTypes"].get("supportAll", False)) else "false"},
            }, 3)
            .bind<DTypeSupport>({
                ${"true" if block["supportedOutputTypes"].get("supportInt", block["supportedOutputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedOutputTypes"].get("supportUInt", block["supportedOutputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedOutputTypes"].get("supportFloat", block["supportedOutputTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedOutputTypes"].get("supportComplexFloat", block["supportedOutputTypes"].get("supportAll", False)) else "false"},
            }, 4)
    %else:
        Pothos::Callable(&TwoToOneBlock::makeFromOneType)
            .bind<TwoToOneFunc>(&af::${block["func"]}, 0)
            .bind<DTypeSupport>({
                ${"true" if block["supportedTypes"].get("supportInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportUInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportComplexFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
            }, 2)
    %endif
    ),
%endfor
};
