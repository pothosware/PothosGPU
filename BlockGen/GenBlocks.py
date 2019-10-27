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

def populateTemplates():
    global FactoryTemplate

    cppFactoryFunctionTemplatePath = os.path.join(ScriptDir, "Factory.mako.cpp")
    with open(cppFactoryFunctionTemplatePath) as f:
        FactoryTemplate = f.read()

def processYAMLFile(yamlPath):
    yml = None
    with open(yamlPath) as f:
        yml = yaml.load(f.read())

    if not yml:
        raise RuntimeError("No YAML found in {0}".format(yamlPath))

    return yml

def generateFactory(blockYAML):
    prefix = """// Copyright (c) 2019-{0} Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

//
// This file was auto-generated on {1}.
//
""".format(Now.year, Now)

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

'''
def generateBlockExecutionTest(expandedYAML):
    sfinaeMap = dict(
        Integer="int",
        UnsignedInt="uint",
        Float="float",
        Complex="complex"
    )
    badParamsMap = dict(
        Integer="badIntValues",
        UnsignedInt="badUIntValues",
        Float="badFloatValues",
        Complex="badComplexValues"
    )

    maxNumParams = max([len(expandedYAML[block].get("funcArgs", [])) for block in expandedYAML])
    assert(maxNumParams > 0)

    try:
        output = Template(BlockExecutionTestTemplate).render(blockYAML=expandedYAML, Now=Now, sfinaeMap=sfinaeMap, maxNumParams=maxNumParams, badParamsMap=badParamsMap)
    except:
        print(mako.exceptions.text_error_template().render())

    outputFilepath = os.path.join(OutputDir, "BlockExecutionTestAuto.cpp")
    with open(outputFilepath, 'w') as f:
        f.write(output)
'''

if __name__ == "__main__":
    populateTemplates()

    yamlFilepath = os.path.join(ScriptDir, "Blocks.yaml")
    blockYAML = processYAMLFile(yamlFilepath)
    generateFactory(blockYAML)
