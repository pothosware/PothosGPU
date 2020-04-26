########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 3.3)
project(PothosArrayFire CXX)

find_package(Pothos 0.7 CONFIG REQUIRED)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

########################################################################
# Find ArrayFire
########################################################################
find_package(ArrayFire 3.6.0 REQUIRED)

########################################################################
# Find Python modules used to implement blocks
########################################################################
find_package(PythonInterp REQUIRED)
include(FindPythonModule)
find_python_module(mako REQUIRED)
find_python_module(yaml REQUIRED)

########################################################################
# json.hpp header
########################################################################
find_path(JSON_HPP_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)

if (NOT JSON_HPP_INCLUDE_DIR)
    message(FATAL_ERROR "Pothos ArrayFire toolkit requires json.hpp")
endif (NOT JSON_HPP_INCLUDE_DIR)

########################################################################
# Auto-generate the majority of blocks
########################################################################

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/BlockGen)
set(autogenDeps
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockGen/Blocks.yaml
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockGen/Factory.mako.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockGen/BlockExecutionTestAuto.mako.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockGen/GenBlocks.py)
set(autogenOutputs
    ${CMAKE_CURRENT_BINARY_DIR}/BlockGen/Factory.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/BlockGen/BlockExecutionTestAuto.cpp)
add_custom_command(
    OUTPUT ${autogenOutputs}
    DEPENDS ${autogenDeps}
    COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/BlockGen/GenBlocks.py" "${CMAKE_CURRENT_BINARY_DIR}/BlockGen" "${ArrayFire_VERSION}"
    COMMENT "Generating block factories and block execution tests")
add_custom_target(
    autogen_files ALL
    DEPENDS ${autogenOutputs})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/Testing
    ${JSON_HPP_INCLUDE_DIR}
    ${ArrayFire_INCLUDE_DIRS})

include(PothosUtil)
POTHOS_MODULE_UTIL(
    TARGET ArrayFireBlocks
    SOURCES
        ${autogenOutputs}

        Source/ArrayFireBlock.cpp
        Source/ArrayFireInfo.cpp
        Source/ArrayOpBlock.cpp
        Source/BufferConversions.cpp
        Source/Cast.cpp
        Source/Clamp.cpp
        Source/Complex.cpp
        Source/Constant.cpp
        Source/Convolve.cpp
        Source/CorrCoef.cpp
        Source/Covariance.cpp
        Source/DeviceCache.cpp
        Source/EnumConversions.cpp
        Source/FactoryOnly.cpp
        Source/FFT.cpp
        Source/FileSink.cpp
        Source/FileSource.cpp
        Source/Filter.cpp
        Source/Flat.cpp
        Source/IsX.cpp
        Source/LogN.cpp
        Source/MinMax.cpp
        Source/NToOneBlock.cpp
        Source/NumericConversions.cpp
        Source/ObjectFunctions.cpp
        Source/OneToOneBlock.cpp
        Source/PinnedBufferManager.cpp
        Source/Pow.cpp
        Source/PowersOfN.cpp
        Source/Random.cpp
        Source/ReducedBlock.cpp
        Source/Replace.cpp
        Source/Root.cpp
        Source/ScalarOpBlock.cpp
        Source/Sort.cpp
        Source/Statistics.cpp
        Source/TopK.cpp
        Source/TwoToOneBlock.cpp
        Source/Utility.cpp

        Testing/ArrayOpBlockTests.cpp
        Testing/BlockExecutionTests.cpp
        Testing/CastBlockTest.cpp
        Testing/ClampBlockTest.cpp
        Testing/ComparatorBlockTest.cpp
        Testing/ComplexBlockTest.cpp
        Testing/FlatBlockTest.cpp
        Testing/IsXBlockTest.cpp
        Testing/ScalarOpBlockTests.cpp
        Testing/ReplaceBlockTest.cpp
        Testing/NToOneBlockExecutionTest.cpp
        Testing/TestObjectFunctions.cpp
        Testing/OneToOneBlockExecutionTest.cpp
        Testing/TwoToOneBlockExecutionTest.cpp
        Testing/TestArrayFireConfig.cpp
        Testing/TestBufferCombos.cpp
        Testing/TestBufferConversions.cpp
        Testing/TestEnumConversions.cpp
        Testing/TestFFT.cpp
        Testing/TestFileSink.cpp
        Testing/TestFileSource.cpp
        Testing/TestManagedDeviceCache.cpp
        Testing/TestNumericConversions.cpp
        Testing/TestStatistics.cpp
        Testing/TestUtility.cpp
    LIBRARIES
        ArrayFire::af
    DOC_SOURCES
        Source/ArrayOpBlock.cpp
        Source/Cast.cpp
        Source/Clamp.cpp
        Source/Complex.cpp
        Source/Constant.cpp
        Source/Convolve.cpp
        Source/CorrCoef.cpp
        Source/Covariance.cpp
        Source/FFT.cpp
        Source/FileSink.cpp
        Source/FileSource.cpp
        Source/Filter.cpp
        Source/Flat.cpp
        Source/IsX.cpp
        Source/LogN.cpp
        Source/MinMax.cpp
        Source/Pow.cpp
        Source/PowersOfN.cpp
        Source/Random.cpp
        Source/Replace.cpp
        Source/Root.cpp
        Source/ScalarOpBlock.cpp
        Source/Sort.cpp
        Source/Statistics.cpp
        Source/TopK.cpp
    DESTINATION arrayfire
)
