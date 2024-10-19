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

#ifndef BF_P4C_IR_IR_ENUMS_H_
#define BF_P4C_IR_IR_ENUMS_H_

#include <lib/cstring.h>

#include <iostream>

namespace P4 {
namespace IR {
namespace MAU {
enum class DataAggregation { NONE, PACKETS, BYTES, BOTH, AGGR_TYPES };
enum class MeterType {
    UNUSED = 0,
    // we use explicit constants here to map to the meter types defined by hardware
    // these are (currently) consistent across tofino1/2, but perhaps should be
    // moved to the Device model
    COLOR_BLIND = 2,
    SELECTOR = 4,
    COLOR_AWARE = 6,
    STFUL_INST0 = 1,
    STFUL_INST1 = 3,
    STFUL_INST2 = 5,
    STFUL_INST3 = 7,
    STFUL_CLEAR = 6,
    METER_TYPES = 8
};
enum class StatefulUse {
    NO_USE,
    DIRECT,
    INDIRECT,
    LOG,
    STACK_PUSH,
    STACK_POP,
    FIFO_PUSH,
    FIFO_POP,
    FAST_CLEAR,
    STFUL_TYPES
};
enum class AddrLocation { DIRECT, OVERHEAD, HASH, STFUL_COUNTER, GATEWAY_PAYLOAD, NOT_SET };
enum class PfeLocation { DEFAULT, OVERHEAD, GATEWAY_PAYLOAD, NOT_SET };
enum class TypeLocation { DEFAULT, OVERHEAD, GATEWAY_PAYLOAD, NOT_SET };
enum class ColorMapramAddress { IDLETIME, STATS, MAPRAM_ADDR_TYPES, NOT_SET };
enum class SelectorMode { FAIR, RESILIENT, SELECTOR_MODES };

/// Table attribute for indicating whether the table is an always-run action, an always-run table,
/// or neither.
enum class AlwaysRun {
    /// Indicates that the table is not always-run.
    NONE,
    /// Indicates that the table is an always-run table.
    TABLE,
    /// Indicates that the table represents an always-run action.
    ACTION
};

std::ostream &operator<<(std::ostream &out, const IR::MAU::DataAggregation &d);
bool operator>>(cstring s, IR::MAU::DataAggregation &d);

std::ostream &operator<<(std::ostream &out, const IR::MAU::MeterType &m);
bool operator>>(cstring s, IR::MAU::MeterType &m);

std::ostream &operator<<(std::ostream &out, const IR::MAU::StatefulUse &u);
bool operator>>(cstring s, IR::MAU::StatefulUse &u);

std::ostream &operator<<(std::ostream &out, const IR::MAU::AddrLocation &a);
bool operator>>(cstring s, IR::MAU::AddrLocation &a);

std::ostream &operator<<(std::ostream &out, const IR::MAU::PfeLocation &p);
bool operator>>(cstring s, IR::MAU::PfeLocation &a);

std::ostream &operator<<(std::ostream &out, const IR::MAU::TypeLocation &t);
bool operator>>(cstring s, IR::MAU::TypeLocation &t);

std::ostream &operator<<(std::ostream &out, const IR::MAU::ColorMapramAddress &cma);
bool operator>>(cstring s, IR::MAU::ColorMapramAddress &cma);

std::ostream &operator<<(std::ostream &out, const IR::MAU::SelectorMode &sm);
bool operator>>(cstring s, IR::MAU::SelectorMode &sm);
std::ostream &operator<<(std::ostream &out, const IR::MAU::AlwaysRun &ar);
bool operator>>(cstring s, IR::MAU::AlwaysRun &ar);

}  // end namespace MAU

namespace BFN {

/// The mode of operation of parser checksum units.
enum class ChecksumMode { VERIFY, RESIDUAL, CLOT };

/// The mode of operation of parser writes to PHV
enum class ParserWriteMode { SINGLE_WRITE, BITWISE_OR, CLEAR_ON_WRITE };

std::ostream &operator<<(std::ostream &out, const IR::BFN::ChecksumMode &t);
bool operator>>(cstring s, IR::BFN::ChecksumMode &t);

std::ostream &operator<<(std::ostream &out, const IR::BFN::ParserWriteMode &t);
bool operator>>(cstring s, IR::BFN::ParserWriteMode &t);

}  // end namespace BFN

}  // end namespace IR

}  // end namespace P4

#endif /* BF_P4C_IR_IR_ENUMS_H_ */
