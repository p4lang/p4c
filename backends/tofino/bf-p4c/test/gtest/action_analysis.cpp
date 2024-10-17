/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "bf-p4c/mau/action_analysis.h"
#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "gtest/gtest.h"
#include "test/gtest/helpers.h"
#include "tofino_gtest_utils.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/validate_allocation.h"
#include "bf-p4c/mau/instruction_selection.h"
#include "bf-p4c/mau/validate_actions.h"

namespace P4::Test {
// valid_instruction_constant unit-tests
#if __cplusplus < 201402L && __cpp_binary_literals < 201304
    #warning "Binary literals are required. A C++14 feature with early availability in GCC"
  // We could fall back on boost/utility/binary.hpp
#else
bool valid_instruction_constant(unsigned value, int maxBits, int minBits,
        int C8Size, int constant_size);

namespace {
unsigned rotate(unsigned value, unsigned width, unsigned leftShift) {
    unsigned mask = (1U << width) - 1;
    value &= mask;
    return ((value << leftShift) | (value >> (width-leftShift))) & mask;
}
constexpr unsigned makeMask(unsigned size) {
    return (0xffffffffU) >> (sizeof(unsigned) * 8 - size);
}

constexpr int maxBits = ActionAnalysis::CONST_SRC_MAX;
constexpr int minBits = ActionAnalysis::CONST_SRC_MAX;
constexpr int minBits2 = ActionAnalysis::JBAY_CONST_SRC_MIN;

// Container sizes.
constexpr int C8Size = 8;
constexpr int C16Size = 16;
constexpr int C32Size = 32;
constexpr int C8Mask = makeMask(C8Size);
constexpr int C16Mask = makeMask(C16Size);
constexpr int C32Mask = makeMask(C32Size);

int C8Value(int value) {
    return value & C8Mask;
}
}  // namespace

TEST(valid_instruction_constant, TofinoLiterals) {
    constexpr int min = ~((1U << minBits) - 1);
    constexpr int max = (1U << maxBits) - 1;
    for (int i = min; i <= max; ++i) {
        EXPECT_TRUE(valid_instruction_constant(C8Value(i), maxBits, minBits, C8Size, 6));
        EXPECT_TRUE(valid_instruction_constant(C8Value(i), maxBits, minBits, C8Size, -1));
    }
    // Out or range, disabling rotation
    EXPECT_FALSE(valid_instruction_constant(C8Value(min-1), maxBits, minBits, C8Size, -1));
    EXPECT_FALSE(valid_instruction_constant(C8Value(max+1), maxBits, minBits, C8Size, -1));
}

TEST(valid_instruction_constant, Tofino2Literals) {
    constexpr int min = ~((1U << minBits2) - 1);
    constexpr int max = (1U << maxBits) - 1;
    for (int i = min; i <= max; ++i) {
        EXPECT_TRUE(valid_instruction_constant(C8Value(i), maxBits, minBits2, C8Size, 6));
        EXPECT_TRUE(valid_instruction_constant(C8Value(i), maxBits, minBits2, C8Size, -1));
    }
    // Out or range, disabling rotation
    EXPECT_FALSE(valid_instruction_constant(C8Value(min-1), maxBits, minBits2, C8Size, -1));
    EXPECT_FALSE(valid_instruction_constant(C8Value(max+1), maxBits, minBits2, C8Size, -1));
}

TEST(valid_instruction_constant, width3) {
    constexpr unsigned width = 3;  // will fit +ve range.
    for (unsigned value = 0; value < (1U << width) ; ++value) {
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, width4) {
    constexpr unsigned width = 4;
    for (unsigned value = 0; value < (1U << width) ; ++value) {
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        bool expected = (value > 7)? false : true;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, width5) {
    constexpr unsigned width = 5;
    for (unsigned value = 0; value < (1U << width) ; ++value) {
        bool expected = (value == 0b01001 || value == 0b01101 ||
                         value == 0b10010 || value == 0b10110)? false : true;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        expected = (value == 0b01001 || value == 0b10010 || value == 0b10110 || value == 0b01101 ||
                    value == 0b01011 || value == 0b10001 || value == 0b10101 || value == 0b11010)
                ? false : true;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        expected = (value > 7)? false : true;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate000011) {  // & 0b100001
    constexpr unsigned width = 6;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value = rotate(0b000011, width, ls);
        bool expected = (value == 0b100001)? false : true;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        expected = (value == 0b000011 || value == 0b000110)? true : false;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate0000011) {  // & 0b1000001
    constexpr unsigned width = 7;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value = rotate(0b0000011, width, ls);
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        bool expected = (value == 0b000011 || value == 0b000110)? true : false;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate000001) {
    constexpr unsigned width = 6;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value = rotate(0b000001, width, ls);
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        bool expected = (value == 0b000001 || value == 0b000010 || value == 0b000100)? true:false;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate000101) {  // & 0b010001
    constexpr unsigned width = 6;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value = rotate(0b000101, width, ls);
        bool expected = (value == 0b010001 || value == 0b100010)? false : true;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        expected = (value == 0b000101)? true : false;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate001001) {
    constexpr unsigned width = 6;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value = rotate(0b001001, width, ls);
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate010101) {
    constexpr unsigned width = 6;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value = rotate(0b010101, width, ls);
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate000111) {
    constexpr unsigned width = 6;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value = rotate(0b000111, width, ls);
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        bool expected = (value == 0b110001 || value == 0b100011)? false : true;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        expected = (value == 0b000111)? true : false;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate001111) {
    constexpr unsigned width = 6;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value = rotate(0b001111, width, ls);
        bool expected = (value == 0b011110)? false : true;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate011111) {
    constexpr unsigned width = 6;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value = rotate(0b011111, width, ls);
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        EXPECT_TRUE(valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate010111) {
    constexpr unsigned width = 6;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value = rotate(0b010111, width, ls);
        bool expected = (value == 0b101110 || value == 0b011101)? false : true;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C8Size, width));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C8Size, width));

        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits, C8Size, -1));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C8Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate01011111111111) {
    constexpr unsigned width = 14;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value = rotate(0b01011111111111, width, ls);
        bool expected = (value == 0b10111111111110 || value == 0b01111111111101)? false : true;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C16Size, width));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C16Size, width));

        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits, C16Size, -1));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C16Size, -1));
    }
}

TEST(valid_instruction_constant, Rotate010111111111111111111111111111) {
    constexpr unsigned width = 30;
    for (unsigned ls = 0; ls < width ; ++ls) {
        unsigned value =   rotate(0b010111111111111111111111111111, width, ls);
        bool expected = (value == 0b101111111111111111111111111110 ||
                         value == 0b011111111111111111111111111101)? false : true;
        EXPECT_EQ(expected, valid_instruction_constant(value, maxBits, minBits, C32Size, width));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C32Size, width));

        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits, C32Size, -1));
        EXPECT_FALSE(valid_instruction_constant(value, maxBits, minBits2, C32Size, -1));
    }
}

// N.B. The following tests are not part of regression as they take too long to run.
// However, they are a useful whilst debugging.
#define RUN_CROSS_CHECK_ALGORITHM 0
#if RUN_CROSS_CHECK_ALGORITHM
namespace {
bool run_cross_check_algorithm(int32_t val, int max_bits, int min_bits,
                                int container_size, int constant_size) {
    int32_t tooLarge = (1 << max_bits);
    int32_t tooSmall = ~(1 << min_bits);
    int32_t container_mask = makeMask(container_size);
    int32_t constant_mask = makeMask(constant_size);
    for (int i = 0; i < container_size; ++i) {
        if (val > tooSmall && val < tooLarge) {
            return true;
        }
        int32_t rotBit = (val >> (container_size - 1)) & 1;
        val = ((val << 1) | rotBit) & container_mask;
    }
    val |= ~constant_mask;
    for (int i = 0; i < container_size; ++i) {
        if (val > tooSmall && val < tooLarge) {
            return true;
        }
        int32_t rotBit = (val >> (container_size - 1)) & 1;
        val = (val << 1) | rotBit | ~container_mask;
    }
    return false;
}
}  // namespace

TEST(valid_instruction_constant, CrossCheckAlgorithm8) {
    for (unsigned width = 1; width <= C8Size; ++width) {
        unsigned max = makeMask(width);
        for (unsigned value = 0; value <= max; ++value) {
            bool res1 = valid_instruction_constant(value, maxBits, minBits, C8Size, width);
            bool res2 = run_cross_check_algorithm(value, maxBits, minBits, C8Size, width);
            BUG_CHECK(res1 == res2, "Mismatch %1% != %2%: %3%, %4%, %5%",
                                            res1, res2, value, C8Size, width);
        }
    }
}

TEST(valid_instruction_constant, CrossCheckAlgorithm16) {  // ~4 msec to run
    for (unsigned width = 1; width <= C16Size; ++width) {
        unsigned max = makeMask(width);
        for (unsigned value = 0; value <= max; ++value) {
            bool res1 = valid_instruction_constant(value, maxBits, minBits, C16Size, width);
            bool res2 = run_cross_check_algorithm(value, maxBits, minBits, C16Size, width);
            BUG_CHECK(res1 == res2, "Mismatch %1% != %2%: %3%, %4%, %5%",
                                            res1, res2, value, C16Size, width);
        }
    }
}

TEST(valid_instruction_constant, CrossCheckAlgorithm32) {  // ~350 seconds to run
    for (unsigned width = 1; width <= C32Size; ++width) {
        std::cout << "C32Size  width=" << std::dec << width << "\n";
        unsigned max = makeMask(width);
        for (unsigned value = 0; /*value <= max*/; ++value) {
            bool res1 = valid_instruction_constant(value, maxBits, minBits, C32Size, width);
            bool res2 = run_cross_check_algorithm(value, maxBits, minBits, C32Size, width);
            BUG_CHECK(res1 == res2, "Mismatch %1% != %2%: %3%, %4%, %5%",
                                            res1, res2, value, C32Size, width);
            if (value && value % 0x1000000 == 0)
                std::cout << "    @ value 0x" << std::hex << value << "\n";
            if (value == max) break;  // We are at UINT32_MAX!
        }
    }
}
#endif   // RUN_CROSS_CHECK_ALGORITHM
#endif   // __cpp_binary_literals

}  // namespace P4::Test

namespace P4::Test {
// ActionAnalysisTest unit-tests

namespace {
std::optional<TofinoPipeTestCase> createTest(const std::string& ingressPipeline) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header H { bit<8> field1; bit<8> field2;}
        struct Headers { H h;}
        struct Metadata { H h; }

        parser parse(packet_in packet, out Headers headers, inout Metadata meta,
                     inout standard_metadata_t sm) {
            state start {
                    packet.extract(headers.h);
                    transition accept;
            }
        }

        control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }
        control ingress(inout Headers headers, inout Metadata meta,
                        inout standard_metadata_t sm) {
            %INGRESS_PIPELINE%
        }

        control egress(inout Headers headers, inout Metadata meta,
                       inout standard_metadata_t sm) {
            apply { } }

        control computeChecksum(inout Headers headers, inout Metadata meta) {
            apply { } }

        control deparse(packet_out packet, in Headers headers) {
            apply { packet.emit(headers.h); }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%INGRESS_PIPELINE%", ingressPipeline);

    auto& options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}
}  // namespace

class ActionAnalysisTest : public TofinoBackendTest {};


TEST_F(ActionAnalysisTest, ParallelAction) {
    auto test = createTest(P4_SOURCE(P4Headers::NONE, R"(
        action swap_fields() {
            bit<8> tmp;
            tmp = headers.h.field1;
            headers.h.field1 = headers.h.field2;
            headers.h.field2 = tmp;
        }
        apply {
            swap_fields();
        }
    )"));
    ASSERT_TRUE(test);

    PhvInfo phv;
    ReductionOrInfo red_info;
    PassManager validate = {
            new CollectHeaderStackInfo,
            // Instructions in actions are sequential.
            new CollectPhvInfo(phv),
            new DoInstructionSelection(phv),
            new CollectPhvInfo(phv),
            new BackendCopyPropagation(phv),
            // Instructions in actions are now parallel
            new CollectPhvInfo(phv),
            new ValidateActions(phv, red_info, false, false, false)
    };

    testing::internal::CaptureStderr();
    test->pipe->apply(validate);
    std::string stderr = testing::internal::GetCapturedStderr();
    // we don't expect any warnings, but the two we are interested in are:
    // warning: Action ingress.swap_fields has a read of a field ingress::headers.h.field1; after
    //          it already has been written
    // warning: Instruction selection creates an instruction that the rest of the compiler cannot
    //          correctly interpret
    EXPECT_TRUE(stderr.find("warning") == std::string::npos);
}

}  // namespace P4::Test
