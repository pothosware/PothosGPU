#include "OneToOneBlock.hpp"
#include "ScalarOpBlock.hpp"
#include "SingleOutputSource.hpp"
#include "TwoToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <arrayfire.h>

#include <vector>

%for block in scalarOpBlocks:
/*
 * |PothosDoc ${block.get("niceName", block["func"].title())}
 *
 * Applies the <b>${block["operator"]}</b> operator to all inputs. This block
 * computes all outputs in parallel, using one of the following implementations
 * by priority (based on availability of hardware and underlying libraries).
 * <ol>
 * <li>CUDA (if GPU present)</li>
 * <li>OpenCL (if GPU present)</li>
 * <li>Standard C++ (if no GPU present)</li>
 * </ol>
 *
 * |category /ArrayFire/${block["header"].title()}
 * |keywords ${block["header"]} ${block["func"]}
 * |factory /arrayfire/${block["header"]}/${block["func"]}(dtype,numChannels)
 *
 * |param dtype(Data Type) The block data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,cfloat=1)
 * |default "float64"
 * |preview enable
 *
 * |param numChannels[Num Channels] The number of channels.
 * |default 1
 * |widget SpinBox(minimum=1)
 * |preview disable
 */
ScalarOpBlockFactory(${block["func"]}, ${block["operator"]})

%endfor
static const std::vector<Pothos::BlockRegistry> BlockRegistries =
{
%for block in oneToOneBlocks:
    /*
     * |PothosDoc ${block.get("niceName", block["func"].title())}
     *
     * Calls <b>af::${block["func"]}</b> on all inputs. This block computes all
     * outputs in parallel, using one of the following implementations by priority
     * (based on availability of hardware and underlying libraries).
     * <ol>
     * <li>CUDA (if GPU present)</li>
     * <li>OpenCL (if GPU present)</li>
     * <li>Standard C++ (if no GPU present)</li>
     * </ol>
     *
     * |category /ArrayFire/${block["header"].title()}
     * |keywords ${block["header"]} ${block["func"]}
    %if "supportedTypes" in block:
     * |factory /arrayfire/${block["header"]}/${block["func"]}(dtype,numChannels)
     *
     * |param dtype(Data Type) The block data type.
     * |widget DTypeChooser(${block["supportedTypes"]["dtypeString"]})
     * |default "${block["supportedTypes"]["defaultType"]}"
     * |preview enable
    %else:
     * |factory /arrayfire/${block["header"]}/${block["func"]}(inputDType,outputDType,numChannels)
     *
     * |param inputDType(Input Data Type) The input data type.
     * |widget DTypeChooser(${block["supportedInputTypes"]["dtypeString"]})
     * |default "${block["supportedInputTypes"]["defaultType"]}"
     * |preview enable
     *
     * |param outputDType(Output Data Type) The output data type.
     * |widget DTypeChooser(${block["supportedOutputTypes"]["dtypeString"]})
     * |default "${block["supportedOutputTypes"]["defaultType"]}"
     * |preview enable
    %endif
     *
     * |param numChannels[Num Channels] The number of channels.
     * |default 1
     * |widget SpinBox(minimum=1)
     * |preview disable
     */
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
    /*
     * |PothosDoc ${block.get("niceName", block["func"].title())}
     *
     * Calls <b>af::${block["func"]}</b> to generate outputs. This block uses
     * one ofthe following implementations by priority (based on availability
     * of hardware and underlying libraries).
     * <ol>
     * <li>CUDA (if GPU present)</li>
     * <li>OpenCL (if GPU present)</li>
     * <li>Standard C++ (if no GPU present)</li>
     * </ol>
     *
     * |category /ArrayFire/${block["header"].title()}
     * |keywords ${block["header"]} ${block["func"]}
     * |factory /arrayfire/${block["header"]}/${block["func"]}(dtype)
     *
     * |param dtype(Data Type) The block data type.
     * |widget DTypeChooser(${block["supportedTypes"]["dtypeString"]})
     * |default "${block["supportedTypes"]["defaultType"]}"
     * |preview enable
     */
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
    /*
     * |PothosDoc ${block.get("niceName", block["func"].title())}
     *
     * Calls <b>af::${block["func"]}</b> on all inputs. This block uses one of
     * the following implementations by priority (based on availability of
     * hardware and underlying libraries).
     * <ol>
     * <li>CUDA (if GPU present)</li>
     * <li>OpenCL (if GPU present)</li>
     * <li>Standard C++ (if no GPU present)</li>
     * </ol>
     *
     * |category /ArrayFire/${block["header"].title()}
     * |keywords ${block["header"]} ${block["func"]}
    %if "supportedTypes" in block:
     * |factory /arrayfire/${block["header"]}/${block["func"]}(dtype)
     *
     * |param dtype(Data Type) The block data type.
     * |widget DTypeChooser(${block["supportedTypes"]["dtypeString"]})
     * |default "${block["supportedTypes"]["defaultType"]}"
     * |preview enable
    %else:
     * |factory /arrayfire/${block["header"]}/${block["func"]}(inputDType,outputDType)
     *
     * |param inputDType(Input Data Type) The input data type.
     * |widget DTypeChooser(${block["supportedInputTypes"]["dtypeString"]})
     * |default "${block["supportedInputTypes"]["defaultType"]}"
     * |preview enable
     *
     * |param outputDType(Output Data Type) The output data type.
     * |widget DTypeChooser(${block["supportedOutputTypes"]["dtypeString"]})
     * |default "${block["supportedOutputTypes"]["defaultType"]}"
     * |preview enable
    %endif
     */
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
            .bind<bool>(${"true" if block.get("allowZeroInBuffer1", True) else "false"}, 5)
    %else:
        Pothos::Callable(&TwoToOneBlock::makeFromOneType)
            .bind<TwoToOneFunc>(&af::${block["func"]}, 0)
            .bind<DTypeSupport>({
                ${"true" if block["supportedTypes"].get("supportInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportUInt", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
                ${"true" if block["supportedTypes"].get("supportComplexFloat", block["supportedTypes"].get("supportAll", False)) else "false"},
            }, 2)
            .bind<bool>(${"true" if block.get("allowZeroInBuffer1", True) else "false"}, 3)
    %endif
    ),
%endfor
};
