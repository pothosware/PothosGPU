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
template <typename T>
static inline T verify_${block["func"]}(const T& val)
{
    return ${block["verify"]}(val);
}
    %endif
%endfor
%for block in singleOutputSources:
%endfor
%for block in twoToOneBlocks:
%endfor

%for k,v in sfinaeMap.items():
template <typename T>
static EnableIf${k}<T, void> blockExecutionTest()
{
    %for block in oneToOneBlocks:
        %for topLevelKey in ["supportedTypes"]:
            %if topLevelKey in block:
                %if block[topLevelKey].get("support{0}".format(v), block[topLevelKey].get("supportAll", False)):
        testOneToOneBlock<T>(
            "/arrayfire/${block["header"]}/${block["func"]}",
            1,
            ${"&verify_{0}<T>".format(block["func"]) if "verify" in block else "nullptr"});
        testOneToOneBlock<T>(
            "/arrayfire/${block["header"]}/${block["func"]}",
            3,
            ${"&verify_{0}<T>".format(block["func"]) if "verify" in block else "nullptr"});
                %endif
            %endif
        %endfor
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
