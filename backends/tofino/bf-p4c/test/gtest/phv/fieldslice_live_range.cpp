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

#include "bf-p4c/phv/fieldslice_live_range.h"

#include <boost/algorithm/string/split.hpp>

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "lib/exceptions.h"

namespace P4::Test {

namespace {

PHV::LiveRange make_lr(int start, PHV::FieldUse::use_t start_op, int end,
                       PHV::FieldUse::use_t end_op) {
    return PHV::LiveRange{PHV::StageAndAccess{start, start_op}, PHV::StageAndAccess{end, end_op}};
}

PHV::LiveRangeInfo::OpInfo parse_info(const std::string &s) {
    if (s == "R") {
        return PHV::LiveRangeInfo::OpInfo::READ;
    } else if (s == "W") {
        return PHV::LiveRangeInfo::OpInfo::WRITE;
    } else if (s == "RW") {
        return PHV::LiveRangeInfo::OpInfo::READ_WRITE;
    } else if (s == "L") {
        return PHV::LiveRangeInfo::OpInfo::LIVE;
    } else if (s == "D") {
        return PHV::LiveRangeInfo::OpInfo::DEAD;
    } else {
        BUG("unknown liverange info: %1%", s);
    }
}

PHV::LiveRangeInfo make(const std::string &str) {
    std::list<std::string> ops;
    boost::split(ops, str, boost::is_any_of(" "), boost::token_compress_on);
    BUG_CHECK(ops.size() <= 14,
              "LiveRangeInfo on Tofino can have at most 14 elements: Parser stage0..11 Deparser");

    PHV::LiveRangeInfo rst;
    if (ops.size() == 0) {
        return rst;
    }
    rst.parser() = parse_info(ops.front());
    ops.pop_front();
    if (ops.size() > 12) {
        rst.deparser() = parse_info(ops.back());
        ops.pop_back();
    }
    int i = 0;
    for (const auto &v : ops) {
        rst.stage(i) = parse_info(v);
        i++;
    }
    return rst;
}

}  // namespace

// require TofinoBackendTest to initialize Device::*.
class FieldSliceLiveRangeTest : public TofinoBackendTest {};

TEST_F(FieldSliceLiveRangeTest, can_overlay) {
    struct testcase {
        std::string a;
        std::string b;
        bool can_overlay;
    };
    std::vector<testcase> cases = {
        {"W L L  L R", "W L RW L D", false},
        {"D D W L R", "W R D D D", true},
        {"D D W L R", "W R R D D", true},
        {"D D W  L R", "W R RW R D", false},
        {"D D W L R", "W R R L R", false},
        {"D D W L R", "W R R R D", false},
        {"W L L R D D D", "D D D W L L R", true},
        {"W L L R D D D", "D D W RW L L R", false},
        {"W R D D D D", "W R D D D D", false},
        {"W L L L R D", "D D W L R D", false},
        {"D W L L L R", "W L RW L D", false},
        {"W R L L L R D D D W L L R", "D D D D D W L L L R D D D", true},
        {"D W L L L R D D  D D D D D D", "D D D D D W L RW L L R L L R", true},
        {"W W L L L R D D  D D D D D D", "D D D D D W L RW L L R L L R", true},
    };
    for (const auto &tc : cases) {
        EXPECT_EQ(tc.can_overlay, make(tc.a).can_overlay(make(tc.b)))
            << tc.a << " and " << tc.b << " can_be_overlaid should be: " << tc.can_overlay;
    }
}

TEST_F(FieldSliceLiveRangeTest, disjoint_ranges) {
    using namespace PHV;
    const auto WRITE = PHV::FieldUse::WRITE;
    const auto READ = PHV::FieldUse::READ;
    struct testcase {
        std::string info;
        std::vector<PHV::LiveRange> expected;
    };

    std::vector<testcase> cases = {
        {"W R",
         {
             make_lr(-1, WRITE, 0, READ),
         }},
        {"W L L R",
         {
             make_lr(-1, WRITE, 2, READ),
         }},
        {"W L L L L RW R",
         {
             make_lr(-1, WRITE, 4, READ),
             make_lr(4, WRITE, 5, READ),
         }},
        {"D D D D D D W L L R D D D D",
         {
             make_lr(5, WRITE, 8, READ),
         }},
        {"W L L L L R D D D W L L R",
         {
             make_lr(-1, WRITE, 4, READ),
             make_lr(8, WRITE, 11, READ),
         }},
        {"W L L L L RW R D D W L L R",
         {
             make_lr(-1, WRITE, 4, READ),
             make_lr(4, WRITE, 5, READ),
             make_lr(8, WRITE, 11, READ),
         }},
        {"D R D D D",
         {
             make_lr(0, READ, 0, READ),
         }},
        {"D R W L R D",
         {
             make_lr(0, READ, 0, READ),
             make_lr(1, WRITE, 3, READ),
         }},
        {"D RW L L R D",
         {
             make_lr(0, READ, 0, READ),
             make_lr(0, WRITE, 3, READ),
         }},
        //  0  1  2  3 4  5  6
        {"D RW RW L R RW RW R D D",
         {
             make_lr(0, READ, 0, READ),
             make_lr(0, WRITE, 1, READ),
             make_lr(1, WRITE, 3, READ),
             make_lr(4, READ, 4, READ),
             make_lr(4, WRITE, 5, READ),
             make_lr(5, WRITE, 6, READ),
         }},
        //  0  1  2  3 4  5  6
        {"D RW RW RW R RW RW R D D",
         {
             make_lr(0, READ, 0, READ),
             make_lr(0, WRITE, 1, READ),
             make_lr(1, WRITE, 2, READ),
             make_lr(2, WRITE, 3, READ),
             make_lr(4, READ, 4, READ),
             make_lr(4, WRITE, 5, READ),
             make_lr(5, WRITE, 6, READ),
         }},
        // ignore parser read.
        {"RW L L L R D",
         {
             make_lr(-1, WRITE, 3, READ),
         }},
        /// tailing writes
        {"W D D",
         {
             make_lr(-1, WRITE, -1, WRITE),
         }},
        {"W W L L R",
         {
             make_lr(-1, WRITE, -1, WRITE),
             make_lr(0, WRITE, 3, READ),
         }},
        {"W D D W",
         {
             make_lr(-1, WRITE, -1, WRITE),
             make_lr(2, WRITE, 2, WRITE),
         }},
    };
    for (const auto &tc : cases) {
        auto rst = make(tc.info).disjoint_ranges();
        ASSERT_EQ(tc.expected.size(), rst.size()) << "range unequal length: " << tc.info;
        for (int i = 0; i < int(rst.size()); ++i) {
            EXPECT_EQ(tc.expected[i], rst[i]) << "unequal at " << i << "; case: " << tc.info;
        }
    }
}

TEST_F(FieldSliceLiveRangeTest, merge_invalid_ranges) {
    using namespace PHV;
    const auto WRITE = PHV::FieldUse::WRITE;
    const auto READ = PHV::FieldUse::READ;
    struct testcase {
        std::vector<PHV::LiveRange> input;
        std::vector<PHV::LiveRange> expected;
    };

    std::vector<testcase> cases = {
        {
            {
                make_lr(-1, WRITE, 0, READ),
            },
            {
                make_lr(-1, WRITE, 0, READ),
            },
        },
        {
            {
                make_lr(-1, WRITE, 4, READ),
                make_lr(4, WRITE, 5, READ),
            },
            {
                make_lr(-1, WRITE, 4, READ),
                make_lr(4, WRITE, 5, READ),
            },
        },
        {
            // case: writes are merged
            {
                make_lr(1, WRITE, 1, WRITE),
                make_lr(2, WRITE, 2, WRITE),
                make_lr(3, WRITE, 4, READ),
                make_lr(4, WRITE, 5, READ),
                make_lr(5, WRITE, 5, WRITE),
                make_lr(6, WRITE, 6, WRITE),
            },
            {
                make_lr(1, WRITE, 2, WRITE),
                make_lr(3, WRITE, 4, READ),
                make_lr(4, WRITE, 5, READ),
                make_lr(5, WRITE, 6, WRITE),
            },
        },
        {
            // case: reads are merged
            {
                make_lr(1, READ, 1, READ),
                make_lr(2, READ, 2, READ),
                make_lr(3, WRITE, 5, READ),
                make_lr(6, READ, 6, READ),
                make_lr(8, READ, 8, READ),
            },
            {
                make_lr(1, READ, 2, READ),
                make_lr(3, WRITE, 5, READ),
                make_lr(6, READ, 8, READ),
            },
        },
        {
            // case: multiple (>2) reads are merged
            {
                make_lr(1, READ, 1, READ),
                make_lr(2, READ, 2, READ),
                make_lr(3, READ, 3, READ),
                make_lr(4, READ, 4, READ),
                make_lr(7, WRITE, 9, READ),
                make_lr(10, READ, 10, READ),
                make_lr(11, READ, 11, READ),
            },
            {
                make_lr(1, READ, 4, READ),
                make_lr(7, WRITE, 9, READ),
                make_lr(10, READ, 11, READ),
            },
        },
        {
            // case: read write not mixed
            {
                make_lr(1, READ, 1, READ),
                make_lr(2, READ, 2, READ),
                make_lr(4, WRITE, 4, WRITE),
                make_lr(8, WRITE, 8, WRITE),
                make_lr(9, READ, 9, READ),
                make_lr(11, READ, 11, READ),
            },
            {
                make_lr(1, READ, 2, READ),
                make_lr(4, WRITE, 8, WRITE),
                make_lr(9, READ, 11, READ),
            },
        },
    };
    for (const auto &tc : cases) {
        auto rst = PHV::LiveRangeInfo::merge_invalid_ranges(tc.input);
        ASSERT_EQ(tc.expected.size(), rst.size()) << "range unequal length: " << rst.size();
        for (int i = 0; i < int(rst.size()); ++i) {
            EXPECT_EQ(tc.expected[i], rst[i]) << "unequal at index " << i;
        }
    }
}

}  // namespace P4::Test
