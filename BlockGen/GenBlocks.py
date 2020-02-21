#!/usr/bin/env python

from mako.template import Template
import mako.exceptions

import datetime
import json
import os
import sys
import yaml

ScriptDir = os.path.dirname(__file__)
OutputDir = os.path.abspath(sys.argv[1])
ArrayFireVersion = sys.argv[2]
Now = datetime.datetime.now()

FactoryTemplate = None
BlockExecutionTestAutoTemplate = None

prefix = """// Copyright (c) 2019-{0} Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

//
// This file was auto-generated on {1}.
//
""".format(Now.year, Now)

def afVersionToAPI(version):
    versionComps = version.split(".")
    assert(len(versionComps) == 3)

    return int("{0}{1}".format(versionComps[0], versionComps[1]))

def populateTemplates():
    global FactoryTemplate
    global BlockExecutionTestAutoTemplate

    factoryFunctionTemplatePath = os.path.join(ScriptDir, "Factory.mako.cpp")
    with open(factoryFunctionTemplatePath) as f:
        FactoryTemplate = f.read()

    blockExecutionTestAutoTemplatePath = os.path.join(ScriptDir, "BlockExecutionTestAuto.mako.cpp")
    with open(blockExecutionTestAutoTemplatePath) as f:
        BlockExecutionTestAutoTemplate = f.read()

# In place
def setBlockNames(blockTypeYAML):
    # If a specific block registry path node name is not provided, use the
    # function name.
    for block in blockTypeYAML:
        if "blockName" not in block:
            block["blockName"] = block["func"]

def filterBlockYAML(blockTypeYAML, printSkippedBlocks=False):
    apiVersion = afVersionToAPI(ArrayFireVersion)
    filteredYAML = [entry for entry in blockTypeYAML if entry.get("minAPIVersion", 0) <= apiVersion]

    if (len(filteredYAML) != blockTypeYAML) and printSkippedBlocks:
        for block in blockTypeYAML:
            if block.get("minAPIVersion", 0) > apiVersion:
                print("Skipping /arrayfire/{0}/{1}, requires API {2} > {3}".format(block["header"], block["func"], block["minAPIVersion"], apiVersion))

    return filteredYAML

def processYAMLFile(yamlPath):
    yml = None
    with open(yamlPath) as f:
        yml = yaml.load(f.read())

    if not yml:
        raise RuntimeError("No YAML found in {0}".format(yamlPath))

    for _,v in yml.items():
        setBlockNames(v)

    return yml

DICT_ENTRY_KEYS = dict(
    supportInt="int16=1,int32=1,int64=1",
    supportUInt="uint=1",
    supportFloat="float=1",
    supportComplexFloat="cfloat=1"
)

# Operates in-place
def generateDTypeDictEntries(supportedTypes):
    if "supportAll" in supportedTypes:
        supportedTypes["dtypeString"] = ",".join([DICT_ENTRY_KEYS[key] for key in DICT_ENTRY_KEYS])
        supportedTypes["defaultType"] = "float64"
    else:
        supportedTypes["dtypeString"] = ",".join([DICT_ENTRY_KEYS[key] for key in DICT_ENTRY_KEYS if key in supportedTypes])
        supportedTypes["defaultType"] = "float64" if ("float=1" in supportedTypes["dtypeString"] and "cfloat=1" not in supportedTypes["dtypeString"]) else "{0}64".format(supportedTypes["dtypeString"].split("=")[0].split(",")[0]).replace("cfloat64", "complex_float64")

def generatePothosDoc(category,blockYAML):
    desc = dict()
    desc["name"] = blockYAML.get("niceName", blockYAML["func"].title())
    desc["path"] = "/arrayfire/{0}/{1}".format(blockYAML["header"], blockYAML["func"])
    desc["categories"] = ["/ArrayFire/" + blockYAML["header"].title()]
    desc["keywords"] = [blockYAML["header"], blockYAML["func"]]
    if "keywords" in blockYAML:
        desc["keywords"] += blockYAML["keywords"]
        desc["keywords"] = list(set(desc["keywords"]))
    if "description" in blockYAML:
        desc["docs"] = [blockYAML["description"]]
    else:
        desc["docs"] = []

    if not blockYAML.get("testOnly", False):
        desc["docs"] += ["<p>Corresponding ArrayFire function: <b>af::{0}</b></p>".format(blockYAML["func"])]

    # Common args for all auto-generated blocks

    desc["params"] = [dict(key="device",
                           name="Device",
                           desc=["ArrayFire device"],
                           preview="enable",
                           default="\"Auto\"")]

    dtypeArg = dict(key="dtype",
                    name="Data Type",
                    desc=["Block data type"],
                    widgetType="DTypeChooser",
                    preview="disable",
                    default="\"int32\"" if blockYAML.get("intOnly", False) else "\"float64\"")
    if "blockPattern" in blockYAML:
        if blockYAML["blockPattern"] in ["FloatToComplex", "ComplexToFloat"]:
            dtypeArg["widgetKwargs"] = dict(float=1)
        else:
            raise RuntimeError("Unknown block pattern: " + blockYAML["blockPattern"])
    elif blockYAML.get("intOnly", False):
        dtypeArg["widgetKwargs"]= dict(int16=1,int32=1,int64=1,uint=1)
    else:
        dtypeArg["widgetKwargs"] = dict()
        if "supportedTypes" in blockYAML:
            supportedTypes = blockYAML["supportedTypes"]
            if ("supportInt" in supportedTypes) or ("supportAll" in supportedTypes):
                dtypeArg["widgetKwargs"].update(dict(int16=1,int32=1,int64=1))
            if ("supportUInt" in supportedTypes) or ("supportAll" in supportedTypes):
                dtypeArg["widgetKwargs"]["uint"] = 1
            if ("supportFloat" in supportedTypes) or ("supportAll" in supportedTypes):
                dtypeArg["widgetKwargs"]["float"] = 1
            if ("supportComplex" in supportedTypes) or ("supportAll" in supportedTypes):
                dtypeArg["widgetKwargs"]["cfloat"] = 1
            dtypeArg["widgetKwargs"]["dim"] = 1

    desc["params"] += [dtypeArg]
    desc["args"] = ["device","dtype"]

    if category == "ScalarOpBlocks":
        desc["params"] += [dict(key="scalar",
                                name="Scalar",
                                desc=["The scalar value to apply to all inputs."],
                                default="0" if blockYAML.get("allowZeroScalar",True) else "1",
                                preview="enable")]
    if category == "NToOneBlocks":
        desc["params"] += [dict(key="numInputs",
                                name="Num Inputs",
                                desc=["The number of inputs for this block."],
                                default="2" if category == "NToOneBlocks" else "1",
                                preview="disable")]

    # Encode the block description into escaped JSON
    descEscaped = "".join([hex(ord(ch)).replace("0x", "\\x") for ch in json.dumps(desc)])
    return "Pothos::PluginRegistry::add(\"/blocks/docs{0}\", std::string(\"{1}\"));".format(desc["path"], descEscaped)

def generateFactory(allBlockYAML):
    # Generate the type support strings here (easier to do than in the Mako
    # template files).
    docs = []
    for category,blocks in allBlockYAML.items():
        for block in blocks:
            for key in ["supportedTypes", "supportedInputTypes", "supportedOutputTypes"]:
                if key in block:
                    generateDTypeDictEntries(block[key])
            docs += [generatePothosDoc(category,block)]

    try:
        rendered = Template(FactoryTemplate).render(
                       oneToOneBlocks=filterBlockYAML([block for block in allBlockYAML["OneToOneBlocks"] if not block.get("testOnly", False)]),
                       scalarOpBlocks=[],
                       singleOutputSources=[],
                       #scalarOpBlocks=filterBlockYAML([block for block in allBlockYAML["ScalarOpBlocks"] if not block.get("testOnly", False)]),
                       #singleOutputSources=filterBlockYAML([block for block in allBlockYAML["SingleOutputSources"] if not block.get("testOnly", False)]),
                       twoToOneBlocks=filterBlockYAML([block for block in allBlockYAML["TwoToOneBlocks"] if not block.get("testOnly", False)]),
                       NToOneBlocks=filterBlockYAML([block for block in allBlockYAML["NToOneBlocks"] if not block.get("testOnly", False)]),
                       docs=docs)
    except:
        print(mako.exceptions.text_error_template().render())

    output = "{0}\n{1}".format(prefix, rendered)

    outputFilepath = os.path.join(OutputDir, "Factory.cpp")
    with open(outputFilepath, 'w') as f:
        f.write(output)

def generateBlockExecutionTest(allBlockYAML):
    sfinaeMap = dict(
        Integer="Int",
        UnsignedInt="UInt",
        Float="Float",
        Complex="ComplexFloat"
    )

    try:
        rendered = Template(BlockExecutionTestAutoTemplate).render(
                       oneToOneBlocks=filterBlockYAML(allBlockYAML["OneToOneBlocks"], True),
                       scalarOpBlocks=[],
                       singleOutputSources=[],
                       #scalarOpBlocks=filterBlockYAML(allBlockYAML["ScalarOpBlocks"], True),
                       #singleOutputSources=filterBlockYAML(allBlockYAML["SingleOutputSources"], True),
                       twoToOneBlocks=filterBlockYAML(allBlockYAML["TwoToOneBlocks"], True),
                       NToOneBlocks=filterBlockYAML(allBlockYAML["NToOneBlocks"], True),
                       sfinaeMap=sfinaeMap)
    except:
        print(mako.exceptions.text_error_template().render())

    output = "{0}\n{1}".format(prefix, rendered)

    outputFilepath = os.path.join(OutputDir, "BlockExecutionTestAuto.cpp")
    with open(outputFilepath, 'w') as f:
        f.write(output)

if __name__ == "__main__":
    populateTemplates()

    yamlFilepath = os.path.join(ScriptDir, "Blocks.yaml")
    allBlockYAML = processYAMLFile(yamlFilepath)

    generateFactory(allBlockYAML)
    generateBlockExecutionTest(allBlockYAML)
