/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_CONSTANTS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_CONSTANTS_H_

#include <cstdint>

#include "ir/ir.h"

namespace P4::P4Tools::P4Testgen::Tofino {

class SharedTofinoConstants {
 public:
    // The minimum packet sizes required to run a successful PTF test.
    static constexpr int64_t PTF_INPUT_PKT_MIN_SIZE = 512;
    static constexpr int64_t PTF_OUTPUT_PKT_MIN_SIZE = 240;
    /// Entries that can match a range.
    static constexpr const char *MATCH_KIND_RANGE = "range";
    /// A match that is used as an argument for the selector.
    static constexpr const char *MATCH_KIND_SELECTOR = "selector";
    /// A match that is as an argument for the atcam index.
    static constexpr const char *MATCH_KIND_ATCAM = "atcam_partition_index";
    /// The port value that corresponds to a dropped packet.
    static constexpr int DROP_CONSTANT = 511;
    /// Bitmask which indicates which ports allowed to be used by generated tests.
    /// Particularly useful for test environments which do not enable all ports to
    /// be used.
    // Seven least significant bits of port number determine the physical port (the rest
    // determines pipe number). 0x7f allows all ports of the first pipe.
    // For now, target only the first pipe.
    // TODO: allow the mask to be specified by a parameter
    static constexpr size_t VALID_INPUT_PORT_MASK = 0x7f;
};

class TofinoConstants {
 public:
    // Parser error codes, copied from tofino.p4.
    static constexpr int PARSER_ERROR_OK = 0x0000;
    static constexpr int PARSER_ERROR_NO_MATCH = 0x0001;
    static constexpr int PARSER_ERROR_PARTIAL_HDR = 0x0002;
    static constexpr int PARSER_ERROR_CTR_RANGE = 0x0004;
    static constexpr int PARSER_ERROR_TIMEOUT_USER = 0x0008;
    static constexpr int PARSER_ERROR_TIMEOUT_HW = 0x0010;
    static constexpr int PARSER_ERROR_SRC_EXT = 0x0020;
    static constexpr int PARSER_ERROR_DST_CONT = 0x0040;
    static constexpr int PARSER_ERROR_PHV_OWNER = 0x0080;
    static constexpr int PARSER_ERROR_MULTIWRITE = 0x0100;
    static constexpr int PARSER_ERROR_ARAM_MBE = 0x0400;
    static constexpr int PARSER_ERROR_FCS = 0x0800;
    // The format of the port metadata appears to be unspecified, but is 64 bits on Tofino.
    static constexpr int PORT_METADATA_SIZE = 64;
    // Size of the frame check sequence appended to the packet.
    static constexpr int64_t ETH_FCS_SIZE = 32;
    // The variable corresponding to the parameter that controls packet drops.
    static const IR::Member INGRESS_DROP_VAR;
    static const IR::Member EGRESS_DROP_VAR;
    /// Port width in bits.
    static constexpr int PORT_BIT_WIDTH = 9;
};

class JBayConstants {
 public:
    // Parser error codes, copied from tofino2.p4.
    static constexpr int PARSER_ERROR_OK = 0x0000;
    static constexpr int PARSER_ERROR_NO_MATCH = 0x0001;
    static constexpr int PARSER_ERROR_PARTIAL_HDR = 0x0002;
    static constexpr int PARSER_ERROR_CTR_RANGE = 0x0004;
    static constexpr int PARSER_ERROR_TIMEOUT_USER = 0x0008;
    static constexpr int PARSER_ERROR_TIMEOUT_HW = 0x0010;
    static constexpr int PARSER_ERROR_SRC_EXT = 0x0020;
    static constexpr int PARSER_ERROR_DST_CONT = 0x0040;
    static constexpr int PARSER_ERROR_PHV_OWNER = 0x0080;
    static constexpr int PARSER_ERROR_MULTIWRITE = 0x0100;
    static constexpr int PARSER_ERROR_ARAM_MBE = 0x0400;
    static constexpr int PARSER_ERROR_FCS = 0x0800;
    static constexpr int PARSER_ERROR_CSUM_MBE = 0x1000;
    // The format of the port metadata appears to be unspecified, but is 192 bits on JBay.
    static constexpr int PORT_METADATA_SIZE = 192;
    // Size of the frame check sequence appended to the packet.
    static constexpr int64_t ETH_FCS_SIZE = 32;
    // The variable corresponding to the parameter that controls packet drops.
    static const IR::Member INGRESS_DROP_VAR;
    static const IR::Member EGRESS_DROP_VAR;
    /// Port width in bits.
    static constexpr int PORT_BIT_WIDTH = 9;
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_CONSTANTS_H_ */
