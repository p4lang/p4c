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

#include <iostream>
#include <sstream>
#include "gtest/gtest.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf-p4c/test/utils/super_cluster_builder.h"
#include "bf-p4c/../p4c/lib/log.h"

namespace P4::Test {

class BuildSuperCluster : public TofinoBackendTest {};

TEST_F(BuildSuperCluster, basic) {
    SuperClusterBuilder scb;

    // Input data
    std::istringstream input_super_cluster(R"(SUPERCLUSTER Uid: 41
    slice lists:	
        [ egress::Twain.Wesson.Buckeye<24> ^0 ^bit[0..47] deparsed exact_containers [0:23]
          egress::Twain.Wesson.Algodones<24> ^0 ^bit[0..23] deparsed exact_containers [0:23] ]
        [ egress::Twain.Mather.Buckeye<24> ^0 deparsed exact_containers [0:23]
          egress::Twain.Mather.Algodones<24> ^0 deparsed exact_containers [0:23] ]
        [ egress::Twain.Westbury.Lacona<12> ^0 deparsed exact_containers [0:11]
          egress::Twain.Westbury.Horton<4> ^4 deparsed exact_containers [0:3]
          egress::Twain.Westbury.Cecilton<1> ^0 deparsed exact_containers [0:0]
          egress::Twain.Westbury.Idalia<1> ^1 deparsed exact_containers [0:0]
          egress::Twain.Westbury.LaPalma<1> ^2 deparsed exact_containers [0:0]
          egress::Twain.Westbury.Ronda<3> ^3 deparsed exact_containers [0:2]
          egress::Twain.Westbury.Laurelton<2> ^6 deparsed exact_containers [0:1]
          egress::Twain.Westbury.Dugger<8> ^0 deparsed exact_containers [0:7] ]
        [ egress::Twain.Hillsview.Keyes<24> ^0 ^bit[0..87] [0:23]
          egress::Twain.Hillsview.PineCity<32> ^0 ^bit[0..63] [0:15]
          egress::Twain.Hillsview.PineCity<32> ^0 ^bit[0..47] [16:16]
          egress::Twain.Hillsview.PineCity<32> ^1 ^bit[0..46] [17:17]
          egress::Twain.Hillsview.PineCity<32> ^2 ^bit[0..45] [18:18]
          egress::Twain.Hillsview.PineCity<32> ^3 ^bit[0..44] [19:19]
          egress::Twain.Hillsview.PineCity<32> ^4 ^bit[0..43] [20:31] ]
        [ egress::Twain.Hillsview.Oriskany<8> ^0 ^bit[0..119] [0:7]
          egress::Twain.Hillsview.Cabot<24> ^0 ^bit[0..111] [0:23] ]
        [ egress::Twain.Hillsview.Marfa<12> ^0 ^bit[0..167] [0:11]
          egress::Twain.Hillsview.__pad_3<4> [0:3] ]
    rotational clusters:	
        [[egress::Twain.Hillsview.Keyes<24> ^0 ^bit[0..87] [0:23]], [egress::Twain.Wesson.Buckeye<24> ^0 ^bit[0..47] deparsed exact_containers [0:23]], [egress::Twain.Mather.Buckeye<24> ^0 deparsed exact_containers [0:23]]]
        [[egress::Twain.Hillsview.PineCity<32> ^0 ^bit[0..63] [0:15]]]
        [[egress::Twain.Hillsview.PineCity<32> ^0 ^bit[0..47] [16:16]]]
        [[egress::Twain.Hillsview.PineCity<32> ^1 ^bit[0..46] [17:17]]]
        [[egress::Twain.Hillsview.PineCity<32> ^2 ^bit[0..45] [18:18]]]
        [[egress::Twain.Hillsview.PineCity<32> ^3 ^bit[0..44] [19:19]]]
        [[egress::Twain.Hillsview.PineCity<32> ^4 ^bit[0..43] [20:31]]]
        [[egress::Twain.Westbury.Dugger<8> ^0 deparsed exact_containers [0:7]], [egress::Twain.Hillsview.Oriskany<8> ^0 ^bit[0..119] [0:7]]]
        [[egress::Twain.Hillsview.Cabot<24> ^0 ^bit[0..111] [0:23]], [egress::Twain.Wesson.Algodones<24> ^0 ^bit[0..23] deparsed exact_containers [0:23]], [egress::Twain.Mather.Algodones<24> ^0 deparsed exact_containers [0:23]]]
        [[egress::Twain.Westbury.Lacona<12> ^0 deparsed exact_containers [0:11]], [egress::Twain.Hillsview.Marfa<12> ^0 ^bit[0..167] [0:11]]]
        [[egress::Twain.Hillsview.__pad_3<4> [0:3]]]
        [[egress::Twain.Westbury.Horton<4> ^4 deparsed exact_containers [0:3]]]
        [[egress::Twain.Westbury.Cecilton<1> ^0 deparsed exact_containers [0:0]]]
        [[egress::Twain.Westbury.Idalia<1> ^1 deparsed exact_containers [0:0]]]
        [[egress::Twain.Westbury.LaPalma<1> ^2 deparsed exact_containers [0:0]]]
        [[egress::Twain.Westbury.Ronda<3> ^3 deparsed exact_containers [0:2]]]
        [[egress::Twain.Westbury.Laurelton<2> ^6 deparsed exact_containers [0:1]]]
	
 )");

    // Build the supercluster
    std::optional<PHV::SuperCluster*> sc = scb.build_super_cluster(input_super_cluster);

    // Check for errors
    if (!sc) {
        FAIL() << "Failed to build the SuperCluster!";
    }

    // Output the created supercluster
    std::ostringstream output_super_cluster;
    output_super_cluster << *sc;

    std::string input_str = input_super_cluster.str();
    input_str.erase(0, input_str.find("\n") + 1);
    // Also erase the last empty line
    input_str.erase(input_str.size()-3, 3);
    std::string output_str = output_super_cluster.str();
    output_str.erase(0, output_str.find("\n") + 1);
    // Compare input and output
    EXPECT_EQ(output_str.compare(input_str), 0);
}

}   // namespace P4::Test
