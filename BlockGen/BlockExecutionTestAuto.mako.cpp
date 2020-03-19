#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"

#include <Pothos/Testing.hpp>

#include <complex>
#include <cmath>
#include <cstdint>

using namespace PothosArrayFireTests;

%for k,v in sfinaeMap.items():

template <typename T>
static EnableIf${k}<T, void> blockExecutionTest()
{
    %if k == "Complex":
    using Scalar = typename T::value_type;
    %endif

    %for block in oneToOneBlocks:
        %if "supportedTypes" in block:
            %if block["supportedTypes"].get("support{0}".format(v), block["supportedTypes"].get("supportAll", False)):
    testOneToOneBlock<T>("/arrayfire/${block["header"]}/${block["blockName"]}");
            %endif
        %endif
    %endfor

    %for block in twoToOneBlocks:
        %if (block.get("pattern", "") == "FloatToComplex") and (k == "Float"):
    testTwoToOneBlockF2C<T>(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
        ${"false" if block.get("allowZeroInBuffer1", True) else "true"});
        %elif "supportedTypes" in block:
            %if block["supportedTypes"].get("support{0}".format(v), block["supportedTypes"].get("supportAll", False)):
    testTwoToOneBlock<T>(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
        ${"false" if block.get("allowZeroInBuffer1", True) else "true"});
            %endif
        %endif
    %endfor

    %for block in NToOneBlocks:
        %if "supportedTypes" in block:
            %if block["supportedTypes"].get("support{0}".format(v), block["supportedTypes"].get("supportAll", False)):
    testNToOneBlock<T>("/arrayfire/${block["header"]}/${block["blockName"]}", 2);
    testNToOneBlock<T>("/arrayfire/${block["header"]}/${block["blockName"]}", 5);
            %endif
        %endif
    %endfor
}
%endfor

POTHOS_TEST_BLOCK("/arrayfire/tests", test_block_execution)
{
    PothosArrayFireTests::setupTestEnv();
    
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
