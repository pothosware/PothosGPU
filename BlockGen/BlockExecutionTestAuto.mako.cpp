#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"

#include <Pothos/Testing.hpp>

#include <complex>
#include <cmath>
#include <cstdint>

using namespace PothosArrayFireTests;

//
// Verification functions
//
%for block in oneToOneBlocks:
    %if "verify" in block:

template <typename In, typename Out>
static inline Out verify_${block["header"]}_${block["blockName"]}(const In& val)
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
static inline Out verify_${block["header"]}_${block["blockName"]}(const In& val0, const In& val1)
{
    return ${block["verify"]}(val0, val1);
}
    %endif
%endfor
%for block in NToOneBlocks:
    %if "verify" in block:

template <typename In, typename Out>
static inline Out verify_${block["header"]}_${block["blockName"]}(const In& val0, const In& val1)
{
    return ${block["verify"]}(val0, val1);
}
    %endif
%endfor
%for k,v in sfinaeMap.items():

template <typename T>
static EnableIf${k}<T, void> blockExecutionTest()
{
    %if k == "Complex":
    using Scalar = typename T::value_type;
    %endif

    %for block in oneToOneBlocks:
        %if (block.get("pattern", "") == "FloatToComplex") and (k == "Float"):
    testOneToOneBlockF2C<T>(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
        ${"&verify_{0}_{1}<T, std::complex<T>>".format(block["header"], block["blockName"]) if "verify" in block else "nullptr"});
    testOneToOneBlockC2F<Scalar>(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
        ${"&verify_{0}_{1}<T, Scalar>".format(block["header"], block["blockName"]) if "verify" in block else "nullptr"});
        %elif "supportedTypes" in block:
            %if block["supportedTypes"].get("support{0}".format(v), block["supportedTypes"].get("supportAll", False)):
    testOneToOneBlock<T>(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
        ${"&verify_{0}_{1}<T,T>".format(block["header"], block["blockName"]) if "verify" in block else "nullptr"});
            %endif
        %endif
    %endfor

    %for block in twoToOneBlocks:
        %if (block.get("pattern", "") == "FloatToComplex") and (k == "Float"):
    testTwoToOneBlockF2C<T>(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
        ${"&verify_{0}_{1}<T, std::complex<T>>".format(block["header"], block["blockName"]) if "verify" in block else "nullptr"},
        ${"false" if block.get("allowZeroInBuffer1", True) else "true"});
        %elif "supportedTypes" in block:
            %if block["supportedTypes"].get("support{0}".format(v), block["supportedTypes"].get("supportAll", False)):
    testTwoToOneBlock<T>(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
        ${"&verify_{0}_{1}<T,T>".format(block["header"], block["blockName"]) if "verify" in block else "nullptr"},
        ${"false" if block.get("allowZeroInBuffer1", True) else "true"});
            %endif
        %endif
    %endfor

    %for block in NToOneBlocks:
        %if "supportedTypes" in block:
            %if block["supportedTypes"].get("support{0}".format(v), block["supportedTypes"].get("supportAll", False)):
    testNToOneBlock<T>(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
        2,
        ${"&verify_{0}_{1}<T,T>".format(block["header"], block["blockName"]) if "verify" in block else "nullptr"});
    testNToOneBlock<T>(
        "/arrayfire/${block["header"]}/${block["blockName"]}",
        5,
        ${"&verify_{0}_{1}<T,T>".format(block["header"], block["blockName"]) if "verify" in block else "nullptr"});
            %endif
        %endif
    %endfor

    const std::string dtypeName = Pothos::DType(typeid(T)).name();

    testCastBlockForType(dtypeName);
    testClampBlockForType(dtypeName);
    testComparatorBlockForType(dtypeName);
    //testFlatBlockForType(dtypeName);
    testSplitComplexBlockForType(dtypeName);
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
