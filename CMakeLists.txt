########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 3.3)
project(PothosArrayFire CXX)

find_package(Pothos CONFIG REQUIRED)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

########################################################################
# Find ArrayFire
########################################################################
find_package(ArrayFire REQUIRED)

########################################################################
# json.hpp header
########################################################################
find_path(JSON_HPP_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)

if (NOT JSON_HPP_INCLUDE_DIR)
    message(WARNING "Pothos ArrayFire toolkit requires json.hpp, skipping...")
endif (NOT JSON_HPP_INCLUDE_DIR)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${JSON_HPP_INCLUDE_DIR})
#
#
#include(BuildPythonModule)
#BUILD_PYTHON_MODULE(
#    TARGET SciPyBlocks
#    DESTINATION PothosSciPy
#    SOURCES
#        Python/__init__.py
#        Python/Airy.py
#        Python/BaseBlock.py
#        #Python/FFT.py
#        Python/ForwardAndPostLabelBlock.py
#        #Python/Load.py
#        #Python/NToOneBlock.py
#        Python/OneToOneBlock.py
#        #Python/Random.py
#        Python/RegisteredCallHelpers.py
#        #Python/Save.py
#        #Python/SaveZ.py
#        Python/Source.py
#        Python/Special.py
#        Python/TwoToOneBlock.py
#        Python/Utility.py
#        Python/WAV.py
#        ${CMAKE_CURRENT_BINARY_DIR}/BlockGen/CppEntryPoints.py
#    CPP_SOURCES
#        ${CMAKE_CURRENT_BINARY_DIR}/BlockGen/Factory.cpp
#        ${CMAKE_CURRENT_BINARY_DIR}/BlockGen/BlockExecutionTest.cpp
#        Cpp/RegisteredCalls.cpp
#
#        #Testing/TestFFT.cpp
#        #Testing/TestSciPyFileIO.cpp
#        Testing/TestRegisteredCalls.cpp
#    DOC_SOURCES
#        ${autogenOutputs}
#        Python/FFT.py
#)
#add_dependencies(SciPyBlocks autogen_files)
