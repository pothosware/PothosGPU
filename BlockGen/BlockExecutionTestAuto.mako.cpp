#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"

#include <Pothos/Testing.hpp>

#include <complex>
#include <cmath>
#include <cstdint>

//
// Verification functions
//
%for block in oneToOneBlocks:
    %if "verify" in block:

template <typename In, typename Out>
static inline Out verify_${block["func"]}(const In& val)
{
    return ${block["verify"]}(val);
}
    %endif
%endfor
%for block in singleOutputSources:
    %if "verify" in block:

    %endif
%endfor
%for block in twoToOneBlocks:
    %if "verify" in block:

template <typename In, typename Out>
static inline Out verify_${block["func"]}(const In& val0, const In& val1)
{
    return ${block["verify"]}(val0, val1);
}
    %endif
%endfor
%for block in scalarOpBlocks:
    %if "verify" in block:

template <typename In, typename Out>
static inline Out verify_${block["func"]}(const In& val0, const In& val1)
{
    return ${block["verify"]}(val0, val1);
}
    %endif
%endfor
%for k,v in sfinaeMap.items():

template <typename T>
static EnableIf${k}<T, void> blockExecutionTest()
{
    %for block in oneToOneBlocks:
        %if "supportedTypes" in block:
            %if block["supportedTypes"].get("support{0}".format(v), block["supportedTypes"].get("supportAll", False)):
    testOneToOneBlock<T>(
        "/arrayfire/${block["header"]}/${block["func"]}",
        1,
        ${"&verify_{0}<T,T>".format(block["func"]) if "verify" in block else "nullptr"});
    testOneToOneBlock<T>(
        "/arrayfire/${block["header"]}/${block["func"]}",
        3,
        ${"&verify_{0}<T,T>".format(block["func"]) if "verify" in block else "nullptr"});
            %endif
        %elif ("supportedInputTypes" in block) and ("support{0}".format(v) in block["supportedInputTypes"]):
            %if v == "ComplexFloat":
    testOneToOneBlock<T, typename T::value_type>(
        "/arrayfire/${block["header"]}/${block["func"]}",
        1,
        ${"&verify_{0}<T, typename T::value_type>".format(block["func"]) if "verify" in block else "nullptr"});
    testOneToOneBlock<T, typename T::value_type>(
        "/arrayfire/${block["header"]}/${block["func"]}",
        3,
        ${"&verify_{0}<T, typename T::value_type>".format(block["func"]) if "verify" in block else "nullptr"});
            %else:
    testOneToOneBlock<T, std::complex<T>>(
        "/arrayfire/${block["header"]}/${block["func"]}",
        1,
        ${"&verify_{0}<T, std::complex<T>>".format(block["func"]) if "verify" in block else "nullptr"});
    testOneToOneBlock<T, std::complex<T>>(
        "/arrayfire/${block["header"]}/${block["func"]}",
        3,
        ${"&verify_{0}<T, std::complex<T>>".format(block["func"]) if "verify" in block else "nullptr"});
            %endif
        %endif
    %endfor

    %for block in twoToOneBlocks:
        %if "supportedTypes" in block:
            %if block["supportedTypes"].get("support{0}".format(v), block["supportedTypes"].get("supportAll", False)):
    testTwoToOneBlock<T>(
        "/arrayfire/${block["header"]}/${block["func"]}",
        ${"&verify_{0}<T,T>".format(block["func"]) if "verify" in block else "nullptr"},
        ${"false" if block.get("allowZeroInBuffer1", True) else "true"});
            %endif
        %elif ("supportedInputTypes" in block) and ("support{0}".format(v) in block["supportedInputTypes"]):
            %if v == "ComplexFloat":
    testTwoToOneBlock<T, typename T::value_type>(
        "/arrayfire/${block["header"]}/${block["func"]}",
        ${"&verify_{0}<T, typename T::value_type>".format(block["func"]) if "verify" in block else "nullptr"},
        ${"false" if block.get("allowZeroInBuffer1", True) else "true"});
            %else:
    testTwoToOneBlock<T, std::complex<T>>(
        "/arrayfire/${block["header"]}/${block["func"]}",
        ${"&verify_{0}<T, std::complex<T>>".format(block["func"]) if "verify" in block else "nullptr"},
        ${"false" if block.get("allowZeroInBuffer1", True) else "true"});
            %endif
        %endif
    %endfor

    %for block in scalarOpBlocks:
    testScalarOpBlock<T>(
        "/arrayfire/${block["header"]}/${block["func"]}",
        1,
        ${"&verify_{0}<T,T>".format(block["func"]) if "verify" in block else "nullptr"});
    testScalarOpBlock<T>(
        "/arrayfire/${block["header"]}/${block["func"]}",
        3,
        ${"&verify_{0}<T,T>".format(block["func"]) if "verify" in block else "nullptr"});
    %endfor
}
%endfor

POTHOS_TEST_BLOCK("/arrayfire/tests", test_block_execution)
{
    blockExecutionTest<std::int16_t>();
    blockExecutionTest<std::int32_t>();
    blockExecutionTest<std::int64_t>();
    blockExecutionTest<std::uint8_t>();
    blockExecutionTest<std::uint16_t>();
    blockExecutionTest<std::uint32_t>();
    blockExecutionTest<std::uint64_t>();
    blockExecutionTest<float>();
    blockExecutionTest<double>();
    blockExecutionTest<std::complex<float>>();
    blockExecutionTest<std::complex<double>>();
}
