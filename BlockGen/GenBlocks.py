#!/usr/bin/env python

import datetime
from mako.template import Template
import mako.exceptions
import os
import sys
import yaml

ScriptDir = os.path.dirname(__file__)
OutputDir = os.path.abspath(sys.argv[1])
Now = datetime.datetime.now()

FactoryTemplate = None
BlockExecutionTestAutoTemplate = None

prefix = """// Copyright (c) 2019-{0} Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

//
// This file was auto-generated on {1}.
//
""".format(Now.year, Now)

def populateTemplates():
    global FactoryTemplate
    global BlockExecutionTestAutoTemplate

    factoryFunctionTemplatePath = os.path.join(ScriptDir, "Factory.mako.cpp")
    with open(factoryFunctionTemplatePath) as f:
        FactoryTemplate = f.read()

    blockExecutionTestAutoTemplatePath = os.path.join(ScriptDir, "BlockExecutionTestAuto.mako.cpp")
    with open(blockExecutionTestAutoTemplatePath) as f:
        BlockExecutionTestAutoTemplate = f.read()

def processYAMLFile(yamlPath):
    yml = None
    with open(yamlPath) as f:
        yml = yaml.load(f.read())

    if not yml:
        raise RuntimeError("No YAML found in {0}".format(yamlPath))

    return yml

def generateFactory(blockYAML):
    try:
        rendered = Template(FactoryTemplate).render(
                       oneToOneBlocks=blockYAML["OneToOneBlocks"],
                       singleOutputSources=blockYAML["SingleOutputSources"],
                       twoToOneBlocks=blockYAML["TwoToOneBlocks"])
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
                       oneToOneBlocks=blockYAML["OneToOneBlocks"],
                       singleOutputSources=blockYAML["SingleOutputSources"],
                       twoToOneBlocks=blockYAML["TwoToOneBlocks"],
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
