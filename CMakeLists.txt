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
    message(FATAL_ERROR "Pothos ArrayFire toolkit requires json.hpp")
endif (NOT JSON_HPP_INCLUDE_DIR)

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
        Source/ArrayFireBlock.cpp
        Source/ArrayFireInfo.cpp
        Source/BufferConversions.cpp
        Source/Constant.cpp
        Source/EnumConversions.cpp
        Source/MinMax.cpp
        Source/OneToOneBlock.cpp
        Source/SingleOutputSource.cpp
        Source/TwoToOneBlock.cpp
        Source/Utility.cpp

        Testing/TestBufferConversions.cpp
        Testing/TestEnumConversions.cpp
    LIBRARIES
        ArrayFire::af
    DESTINATION arrayfire
)
