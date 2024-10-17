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

#include "bf-p4c/mau/attached_info.h"

#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"

namespace P4::Test {

TEST(AttachedInfo, BuildSplitMaps) {
    // We are using TestCode::Hdr::TofinoMin, so need to copy-paste Register<> definitions.
    auto defs = R"(
        extern Register<T, I> {
            Register(bit<32> size);
            Register(bit<32> size, T initial_value);
            T read(in I index);
            void write(in I index, in T value);
        }
        extern RegisterAction<T, I, U> {
            RegisterAction(Register<_, _> reg);
            U execute(in I index);
            U execute_log();
            @synchronous(execute, execute_log)
            abstract void apply(inout T value, @optional out U rv);
            U predicate(@optional in bool cmplo,
                        @optional in bool cmphi);
        }
        struct Reg1 { bit<8> value; }
        struct Reg2 { bit<16> value; }
        struct headers_t {}
        struct local_metadata_t {}
        )";

    auto input = R"(
        Register<Reg1, bit<8>>(1) reg1;
        RegisterAction<Reg1, bit<8>, bool>(reg1) reg1_action = {
            void apply(inout Reg1 v, out bool b) {
                v.value = 0;
                b = true;
            }
        };
        Register<Reg2, bit<8>>(1) reg2;
        RegisterAction<Reg2, bit<8>, bool>(reg2) reg2_action = {
            void apply(inout Reg2 v, out bool b) {
                v.value = 0;
                b = true;
            }
        };
        action a() {
            reg1_action.execute(0);
            reg2_action.execute(0);
        }
        apply {
            a();
            a();
        })";

    auto blk = TestCode(TestCode::Hdr::TofinoMin, TestCode::tofino_shell(),
                        {defs, TestCode::empty_state(), input, TestCode::empty_appy()});
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullFrontend));
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullMidend));
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::ConverterToBackend));

    // This is an XFAIL.
    // At present table placement only supports the splitting one attached table.
    // See SplitAttachedInfo::BuildSplitMaps
    testing::internal::CaptureStderr();
    EXPECT_FALSE(blk.apply_pass(TestCode::Pass::FullBackend));
    auto stderr = testing::internal::GetCapturedStderr();
    EXPECT_TRUE(stderr.find("error: overlap. Both Register ingress_control.reg2 and "
                            "ingress_control.reg1 require the meter address hardware, "
                            "and cannot be on the same table tbl_a.") != std::string::npos);
}

}  // namespace P4::Test
