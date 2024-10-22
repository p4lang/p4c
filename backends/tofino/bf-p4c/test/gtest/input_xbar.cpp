/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf-p4c/mau/tofino/input_xbar.h"

#include <sstream>

#include "bf-p4c/ir/gress.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"

namespace P4::Test {

class InputXbarAlloc : public TofinoBackendTest {
 protected:
    InputXbarAlloc() {}

    PhvInfo phv;
    safe_vector<IXBar::Use::Byte> alloc;
};

TEST_F(InputXbarAlloc, hello) {
    safe_vector<IXBar::Use::Byte *> alloced;
    Tofino::IXBar::hash_matrix_reqs hm_reqs;
    hm_reqs.max_search_buses = -1;
    hm_reqs.index_groups = 0;
    hm_reqs.select_bits = 0;
    hm_reqs.hash_dist = 0;
    hm_reqs.requires_versioning = 1;

    Tofino::IXBar ixbar;

    alloc.clear();
    auto byte = new IXBar::Use::Byte("W10", 16);
    byte->bit_use.setrange(0, 8);
    byte->flags = 36;
    byte->non_zero_bits.setrange(0, 8);
    alloc.push_back(*byte);
    auto byte2 = new IXBar::Use::Byte("W10", 24);
    byte2->bit_use.setrange(0, 8);
    byte2->flags = 40;
    byte2->non_zero_bits.setrange(0, 8);
    alloc.push_back(*byte2);

    auto &use = ixbar.get_ternary_use();
    use.clear();

    auto &byte_group_use = ixbar.get_byte_group_use();
    byte_group_use.clear();

    // note that ternary ixbar group[4] has one free byte and
    // group[8] has two free bytes.
    //
    // The better allocation scheme is to allocate two bytes into group[8]
    // which will increase tcam memory efficiency, however due to PHV
    // allocation (W10), we need to use tcam byte swizzler to swizzle the input byte
    // into the right offset.
    use[0][0] = {"H46", 0};    // byte(0)
    use[0][1] = {"B10", 0};    // byte(1)
    use[0][2] = {"H74", 0};    // byte(2)
    use[0][3] = {"B33", 0};    // byte(3)
    use[0][4] = {"B7", 0};     // byte(4)
    use[1][0] = {"H3", 0};     // byte(6)
    use[1][1] = {"H36", 8};    // byte(7)
    use[1][2] = {"H36", 0};    // byte(8)
    use[1][3] = {"H39", 8};    // byte(9)
    use[1][4] = {"H39", 0};    // byte(10)
    use[2][0] = {"B32", 0};    // byte(11)
    use[2][1] = {"B41", 0};    // byte(12)
    use[2][2] = {"B42", 0};    // byte(13)
    use[2][3] = {"B56", 0};    // byte(14)
    use[3][0] = {"W6", 8};     // byte(17)
    use[3][1] = {"W6", 16};    // byte(18)
    use[3][2] = {"W6", 24};    // byte(19)
    use[3][3] = {"W6", 0};     // byte(20)
    use[4][0] = {"W13", 16};   // byte(22)
    use[4][1] = {"W13", 24};   // byte(23)
    use[4][2] = {"W13", 0};    // byte(24)
    use[4][3] = {"W13", 8};    // byte(25)
    use[5][0] = {"W35", 0};    // byte(28)
    use[5][1] = {"W35", 8};    // byte(29)
    use[5][2] = {"W35", 16};   // byte(30)
    use[5][3] = {"W35", 24};   // byte(31)
    use[5][4] = {"W40", 0};    // byte(32)
    use[6][0] = {"W42", 8};    // byte(33)
    use[6][1] = {"W42", 16};   // byte(34)
    use[6][2] = {"W42", 24};   // byte(35)
    use[6][3] = {"W44", 0};    // byte(36)
    use[6][4] = {"W44", 8};    // byte(37)
    use[7][0] = {"W44", 24};   // byte(39)
    use[7][1] = {"W52", 0};    // byte(40)
    use[7][2] = {"W52", 8};    // byte(41)
    use[7][3] = {"W44", 16};   // byte(42)
    use[7][4] = {"W52", 24};   // byte(43)
    use[8][0] = {"W11", 0};    // byte(44)
    use[8][1] = {"W11", 8};    // byte(45)
    use[8][2] = {"W5", 16};    // byte(46)
    use[9][0] = {"W52", 16};   // byte(50)
    use[9][1] = {"W6", 24};    // byte(51)
    use[9][2] = {"W6", 0};     // byte(52)
    use[9][3] = {"W6", 8};     // byte(53)
    use[9][4] = {"W6", 16};    // byte(54)
    use[10][0] = {"W40", 24};  // byte(55)
    use[10][1] = {"W41", 0};   // byte(56)
    use[10][2] = {"W40", 8};   // byte(57)
    use[10][3] = {"W40", 16};  // byte(58)
    use[10][4] = {"W41", 24};  // byte(59)
    use[11][0] = {"W41", 8};   // byte(61)
    use[11][1] = {"W41", 16};  // byte(62)
    use[11][2] = {"W5", 24};   // byte(63)
    use[11][3] = {"W5", 0};    // byte(64)
    use[11][4] = {"W5", 8};    // byte(65)

    byte_group_use[0] = {"H3", 8};
    byte_group_use[1] = {"W42", 0};

    bool rv = ixbar.find_alloc(alloc, true, alloced, hm_reqs);

    ASSERT_TRUE(rv);

    // TODO: We can do better in this allocation to use the same group
    ASSERT_FALSE(alloced[0]->loc.group == alloced[1]->loc.group);
}

TEST_F(InputXbarAlloc, TestXbarUseByteVisualizationDetail) {
    IXBar::Use::Byte byte("W10", 16);
    byte.add_info(IXBar::FieldInfo("ingress::hdr.inner_ethernet.$valid"_cs, 0, 0, 3, std::nullopt));
    byte.add_info(IXBar::FieldInfo("ingress::hdr.inner_ipv4.$valid"_cs, 0, 0, 4, std::nullopt));
    byte.add_info(IXBar::FieldInfo("ingress::hdr.inner_tcp.$valid"_cs, 0, 0, 6, std::nullopt));
    byte.add_info(IXBar::FieldInfo("ingress::hdr.inner_udp.$valid"_cs, 0, 0, 7, std::nullopt));

    auto detail = byte.visualization_detail();
    ASSERT_EQ(detail,
              "{unused[0:2], "
              "ingress::hdr.inner_ethernet.$valid[0:0], "
              "ingress::hdr.inner_ipv4.$valid[0:0], "
              "unused[0:0], "
              "ingress::hdr.inner_tcp.$valid[0:0], "
              "ingress::hdr.inner_udp.$valid[0:0]}");
}

}  // namespace P4::Test
