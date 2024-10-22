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

#include "bf-p4c/common/scc_toposort.h"

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"

namespace P4::Test {

class TofinoSccToposort : public TofinoBackendTest {};

TEST_F(TofinoSccToposort, empty) {
    SccTopoSorter sorter;
    auto rst = sorter.scc_topo_sort();
    EXPECT_TRUE(rst.empty());
}

TEST_F(TofinoSccToposort, single_node) {
    /**
       1
     */
    SccTopoSorter sorter;
    sorter.new_node();

    auto rst = sorter.scc_topo_sort();
    EXPECT_EQ(rst.at(1), 1);
}

TEST_F(TofinoSccToposort, circular_single_node) {
    /**
       1 <- 1
     */
    SccTopoSorter sorter;
    sorter.new_node();
    sorter.add_dep(1, 1);

    auto rst = sorter.scc_topo_sort();
    EXPECT_EQ(rst.at(1), 1);
}

TEST_F(TofinoSccToposort, circular_nodes) {
    /**
       1 <- 2 <- 3
       ---------->
     */
    SccTopoSorter sorter;
    for (int i = 1; i <= 3; i++) {
        sorter.new_node();
    }
    sorter.add_dep(2, 1);
    sorter.add_dep(3, 2);
    sorter.add_dep(1, 3);

    auto rst = sorter.scc_topo_sort();
    EXPECT_EQ(rst.at(1), 1);
    EXPECT_EQ(rst.at(2), 1);
    EXPECT_EQ(rst.at(3), 1);
}

TEST_F(TofinoSccToposort, basic_4_nodes) {
    /**
       1 <- 2 <- 3 <-> 4
     */
    SccTopoSorter sorter;
    for (int i = 1; i <= 4; i++) {
        sorter.new_node();
    }
    sorter.add_dep(2, 1);
    sorter.add_dep(3, 2);
    sorter.add_dep(3, 4);
    sorter.add_dep(4, 3);

    auto rst = sorter.scc_topo_sort();
    EXPECT_EQ(rst.at(3), 1);
    EXPECT_EQ(rst.at(4), 1);
    EXPECT_EQ(rst.at(2), 2);
    EXPECT_EQ(rst.at(1), 3);
}

}  // namespace P4::Test
