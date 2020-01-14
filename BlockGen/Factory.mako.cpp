#include "OneToOneBlock.hpp"
#include "NToOneBlock.hpp"
#include "ScalarOpBlock.hpp"
#include "TwoToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Callable.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <arrayfire.h>

#include <vector>

%for block in scalarOpBlocks:
/*
 * |PothosDoc ${block.get("niceName", block["blockName"].title())}
    %if "description" in block:
 *
${block["description"]}
    %endif
 *
 * Applies the <b>${block["operator"]}</b> operator to all inputs, with a given
 * scalar value. This block computes all outputs in parallel, using one of the
 * following implementations by priority (based on availability of hardware
 * and underlying libraries).
 * <ol>
 * <li>CUDA (if GPU present)</li>
 * <li>OpenCL (if GPU present)</li>
 * <li>Standard C++ (if no GPU present)</li>
 * </ol>
 *
 * |category /ArrayFire/${block["header"].title()}
 * |keywords ${block["header"]} ${block["blockName"]}
 * |factory /arrayfire/${block["header"]}/${block["blockName"]}(device,dtype,scalar,numChannels)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 * |widget ComboBox(editable=false)
 * |preview enable
 *
 * |param dtype(Data Type) The block data type.
 * |widget DTypeChooser(${"int16=1,int32=1,int64=1,uint=1" if block.get("intOnly", False) else "int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1"})
 * |default ${"\"uint64\"" if block.get("intOnly", False) else "\"float64\""}
 * |preview disable
 *
 * |param scalar(Scalar Value) The scalar value to apply to the array.
 * |default ${"0" if block.get("allowZeroScalar", True) else "1"}
 *
 * |param numChannels[Num Channels] The number of channels.
 * |default 1
 * |widget SpinBox(minimum=1)
 * |preview disable
 */
ScalarOpBlockFactory(${block["blockName"]}, ${block["operator"]}, ${"true" if block.get("allowZeroScalar", True) else "false"}, ${"true" if block.get("intOnly", False) else "false"})

%endfor
static const std::vector<Pothos::BlockRegistry> BlockRegistries =
{
%for block in oneToOneBlocks:
/*
 * |PothosDoc ${block.get("niceName", block["blockName"].title())}
%if "description" in block:
 *
${block["description"]}
%endif
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
 * |keywords ${block["header"]} ${block["blockName"]}
%if block.get("pattern", "") == "FloatToComplex":
 * |factory /arrayfire/${block["header"]}/${block["blockName"]}(device,floatDType,numChannels)
 *
 * |param floatDType(Data Type) The float type for the scalar input and complex output.
 * |widget DTypeChooser(float=1)
 * |default "float64"
 * |preview disable
%elif block.get("pattern", "") == "ComplexToFloat":
 * |factory /arrayfire/${block["header"]}/${block["blockName"]}(device,floatDType,numChannels)
 *
 * |param floatDType(Data Type) The float type for the complex input and scalar output.
 * |widget DTypeChooser(float=1)
 * |default "float64"
 * |preview disable
%elif "supportedTypes" in block:
 * |factory /arrayfire/${block["header"]}/${block["blockName"]}(device,dtype,numChannels)
 *
 * |param dtype(Data Type) The block data type.
 * |widget DTypeChooser(${block["supportedTypes"]["dtypeString"]})
 * |default "${block["supportedTypes"]["defaultType"]}"
 * |preview disable
%else:
 * |factory /arrayfire/${block["header"]}/${block["blockName"]}(deviceinputDType,outputDType,numChannels)
 *
 * |param inputDType(Input Data Type) The input data type.
 * |widget DTypeChooser(${block["supportedInputTypes"]["dtypeString"]})
 * |default "${block["supportedInputTypes"]["defaultType"]}"
 * |preview disable
 *
 * |param outputDType(Output Data Type) The output data type.
 * |widget DTypeChooser(${block["supportedOutputTypes"]["dtypeString"]})
 * |default "${block["supportedOutputTypes"]["defaultType"]}"
 * |preview disable
%endif
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 * |widget ComboBox(editable=false)
 * |preview enable
 *
 * |param numChannels[Num Channels] The number of channels.
 * |default 1
 * |widget SpinBox(minimum=1)
 * |preview disable
 */
    Pothos::BlockRegistry(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
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
/*
 * |PothosDoc ${block.get("niceName", block["blockName"].title())}
%if "description" in block:
 *
${block["description"]}
%endif
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
 * |keywords ${block["header"]} ${block["blockName"]}
%if block.get("pattern", "") == "FloatToComplex":
 * |factory /arrayfire/${block["header"]}/${block["blockName"]}(device,floatDType)
 *
 * |param floatDType(Data Type) The float type for the scalar input and complex output.
 * |widget DTypeChooser(float=1)
 * |default "float64"
 * |preview disable
%elif "supportedTypes" in block:
 * |factory /arrayfire/${block["header"]}/${block["blockName"]}(device,dtype)
 *
 * |param dtype(Data Type) The block data type.
 * |widget DTypeChooser(${block["supportedTypes"]["dtypeString"]})
 * |default "${block["supportedTypes"]["defaultType"]}"
 * |preview disable
%else:
 * |factory /arrayfire/${block["header"]}/${block["blockName"]}(device,inputDType,outputDType)
 *
 * |param inputDType(Input Data Type) The input data type.
 * |widget DTypeChooser(${block["supportedInputTypes"]["dtypeString"]})
 * |default "${block["supportedInputTypes"]["defaultType"]}"
 * |preview disable
 *
 * |param outputDType(Output Data Type) The output data type.
 * |widget DTypeChooser(${block["supportedOutputTypes"]["dtypeString"]})
 * |default "${block["supportedOutputTypes"]["defaultType"]}"
 * |preview disable
%endif
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 * |widget ComboBox(editable=false)
 * |preview enable
 */
    Pothos::BlockRegistry(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
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
/*
 * |PothosDoc ${block.get("niceName", block["blockName"].title())}
    %if "description" in block:
 *
${block["description"]}
    %elif "operator" in block:
 *
 * Applies the <b>${block["operator"]}</b> operator to all inputs, resulting
 * in a single output.
   %endif
 *
 * This block computes all outputs in parallel, using one
 * of the following implementations by priority (based on availability of
 * hardware and underlying libraries).
 * <ol>
 * <li>CUDA (if GPU present)</li>
 * <li>OpenCL (if GPU present)</li>
 * <li>Standard C++ (if no GPU present)</li>
 * </ol>
 *
 * |category /ArrayFire/${block["header"].title()}
 * |keywords ${block["header"]} ${block["blockName"]}
 * |factory /arrayfire/${block["header"]}/${block["blockName"]}(device,dtype,numChannels)
 *
 * |param device[Device] ArrayFire device to use.
 * |default "Auto"
 * |widget ComboBox(editable=false)
 * |preview enable
 *
 * |param dtype(Data Type) The block data type.
 * |widget DTypeChooser(${"int16=1,int32=1,int64=1,uint=1" if block.get("intOnly", False) else "int16=1,int32=1,int64=1,uint=1,float=1,cfloat=1"})
 * |default ${"\"uint64\"" if block.get("intOnly", False) else "\"float64\""}
 * |preview disable
 *
 * |param numChannels[Num Channels] The number of channels.
 * |default 2
 * |widget SpinBox(minimum=2)
 * |preview disable
 */
    Pothos::BlockRegistry(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
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
    ),
%endfor
};
