#!/usr/bin/env python

import datetime
from mako.template import Template
import mako.exceptions
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

# In place
def commentizeBlockDescriptions(blockTypeYAML):
    for block in blockTypeYAML:
        if "description" in block:
            block["description"] = " * {0}".format(block["description"])

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
        commentizeBlockDescriptions(v)

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
        supportedTypes["dtypeString"] = ",".join([DICT_ENTRY_KEYS[key] for key in DICT_ENTRY_KEYS])+",dim=1"
        supportedTypes["defaultType"] = "float64"
    else:
        supportedTypes["dtypeString"] = ",".join([DICT_ENTRY_KEYS[key] for key in DICT_ENTRY_KEYS if key in supportedTypes])+",dim=1"
        supportedTypes["defaultType"] = "float64" if ("float=1" in supportedTypes["dtypeString"] and "cfloat=1" not in supportedTypes["dtypeString"]) else "{0}64".format(supportedTypes["dtypeString"].split("=")[0].split(",")[0]).replace("cfloat64", "complex_float64")

def generateFactory(blockYAML):
    # Generate the type support strings here (easier to do than in the Mako
    # template files).
    for category,blocks in blockYAML.items():
        for block in blocks:
            for key in ["supportedTypes", "supportedInputTypes", "supportedOutputTypes"]:
                if key in block:
                    generateDTypeDictEntries(block[key])

    try:
        rendered = Template(FactoryTemplate).render(
                       oneToOneBlocks=filterBlockYAML(blockYAML["OneToOneBlocks"]),
                       singleOutputSources=[],
                       twoToOneBlocks=filterBlockYAML(blockYAML["TwoToOneBlocks"]),
                       NToOneBlocks=filterBlockYAML(blockYAML["NToOneBlocks"]),)
    except:
        print(mako.exceptions.text_error_template().render())

    output = "{0}\n{1}".format(prefix, rendered)

    outputFilepath = os.path.join(OutputDir, "Factory.cpp")
    with open(outputFilepath, 'w') as f:
        f.write(output)

# TODO: make OneToOneBlock test support different types
def generateBlockExecutionTest(blockYAML):
    sfinaeMap = dict(
        Integer="Int",
        UnsignedInt="UInt",
        Float="Float",
        Complex="ComplexFloat"
    )

    try:
        rendered = Template(BlockExecutionTestAutoTemplate).render(
                       oneToOneBlocks=filterBlockYAML(blockYAML["OneToOneBlocks"], True),
                       singleOutputSources=[],
                       twoToOneBlocks=filterBlockYAML(blockYAML["TwoToOneBlocks"], True),
                       NToOneBlocks=filterBlockYAML(blockYAML["NToOneBlocks"], True),
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
    blockYAML = processYAMLFile(yamlFilepath)

    generateFactory(blockYAML)
    generateBlockExecutionTest(blockYAML)
