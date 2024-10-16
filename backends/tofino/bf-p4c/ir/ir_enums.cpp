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

#include "bf-p4c/ir/ir_enums.h"

namespace P4 {
namespace IR {
namespace MAU {

static const char *data_aggr_to_str[] = {
    "NONE", "PACKETS", "BYTES", "BOTH"
};

std::ostream &operator<<(std::ostream &out, const IR::MAU::DataAggregation &d) {
    out << data_aggr_to_str[static_cast<int>(d)];
    return out;
}

bool operator>>(cstring s, IR::MAU::DataAggregation &d) {
    if (!s || s == "") {
        d = IR::MAU::DataAggregation::NONE;
        return true;
    }
    for (int i = 0; i < static_cast<int>(IR::MAU::DataAggregation::AGGR_TYPES); i++) {
        if (data_aggr_to_str[i] == s) {
            d = static_cast<IR::MAU::DataAggregation>(i);
            return true;
        }
    }
    return false;
}


static const char *meter_type_to_str[] = {
    "UNUSED", "STFUL_INST0", "COLOR_BLIND", "STFUL_INST1", "SELECTOR", "STFUL_INST2",
    "COLOR_AWARE", "STFUL_INSTR3"
};

std::ostream &operator<<(std::ostream &out, const IR::MAU::MeterType &m) {
    // FIXME -- COLOR_AWARE and STFUL_CLEAR are overloaded into the same code; which it
    // is depends on the context.
    out << meter_type_to_str[static_cast<int>(m)];
    return out;
}

bool operator>>(cstring s, IR::MAU::MeterType &m) {
    if (!s || s == "") {
        m = IR::MAU::MeterType::UNUSED;
        return true;
    }
    for (int i = 0; i < static_cast<int>(IR::MAU::MeterType::METER_TYPES); i++) {
        if (meter_type_to_str[i] == s) {
            m = static_cast<IR::MAU::MeterType>(i);
            return true;
        }
    }
    if (s == "STFUL_CLEAR") {
        m = IR::MAU::MeterType::STFUL_CLEAR;
        return true; }
    return false;
}

static const char *stateful_use_to_str[] = {
    "NO_USE", "DIRECT", "INDIRECT", "LOG", "STACK_PUSH", "STACK_POP", "FIFO_PUSH", "FIFO_POP",
    "FAST_CLEAR", "STFUL_TYPES"
};


std::ostream &operator<<(std::ostream &out, const IR::MAU::StatefulUse &s) {
    out << stateful_use_to_str[static_cast<int>(s)];
    return out;
}

bool operator>>(cstring s, IR::MAU::StatefulUse &u) {
    if (!s || s == "") {
        u = IR::MAU::StatefulUse::NO_USE;
        return true;
    }
    for (int i = 0; i < static_cast<int>(IR::MAU::StatefulUse::STFUL_TYPES); i++) {
        if (stateful_use_to_str[i] == s) {
            u = static_cast<IR::MAU::StatefulUse>(i);
            return true;
        }
    }
    return false;
}


static const char *addr_location_to_str[] = {
    "DIRECT", "OVERHEAD", "HASH", "STFUL_COUNTER", "GATEWAY_PAYLOAD", "NOT_SET"
};

std::ostream &operator<<(std::ostream &out, const IR::MAU::AddrLocation &a) {
    out << addr_location_to_str[static_cast<int>(a)];
    return out;
}

bool operator>>(cstring s, IR::MAU::AddrLocation &a) {
    if (!s || s == "") {
        a = IR::MAU::AddrLocation::NOT_SET;
        return true;
    }
    for (int i = 0; i <= static_cast<int>(IR::MAU::AddrLocation::NOT_SET); i++) {
        if (addr_location_to_str[i] == s) {
            a = static_cast<IR::MAU::AddrLocation>(i);
            return true;
        }
    }
    return false;
}

static const char *pfe_location_to_str[] = {
    "DEFAULT", "OVERHEAD", "GATEWAY_PAYLOAD", "NOT_SET"
};

std::ostream &operator<<(std::ostream &out, const IR::MAU::PfeLocation &p) {
    out << pfe_location_to_str[static_cast<int>(p)];
    return out;
}

bool operator>>(cstring s, IR::MAU::PfeLocation &p) {
    if (!s || s == "") {
        p = IR::MAU::PfeLocation::NOT_SET;
        return true;
    }
    for (int i = 0; i <= static_cast<int>(IR::MAU::PfeLocation::NOT_SET); i++) {
        if (pfe_location_to_str[i] == s) {
            p = static_cast<IR::MAU::PfeLocation>(i);
            return true;
        }
    }
    return false;
}

static const char *type_location_to_str[] = {
    "DEFAULT", "OVERHEAD", "GATEWAY_PAYLOAD", "NOT_SET"
};

std::ostream &operator<<(std::ostream &out, const IR::MAU::TypeLocation &t) {
    out << type_location_to_str[static_cast<int>(t)];
    return out;
}

bool operator>>(cstring s, IR::MAU::TypeLocation &t) {
    if (!s || s == "") {
        t = IR::MAU::TypeLocation::NOT_SET;
        return true;
    }
    for (int i = 0; i <= static_cast<int>(IR::MAU::TypeLocation::NOT_SET); i++) {
        if (type_location_to_str[i] == s) {
            t = static_cast<IR::MAU::TypeLocation>(i); return true;
        }
    }
    return false;
}

static const char *color_mapram_address_to_str[] = {
    "IDLETIME", "STATS", "MAPRAM_ADDR_TYPES", "NOT_SET"
};

std::ostream &operator<<(std::ostream &out, const IR::MAU::ColorMapramAddress &cma) {
    out << color_mapram_address_to_str[static_cast<int>(cma)];
    return out;
}

bool operator>>(cstring s, IR::MAU::ColorMapramAddress &cma) {
    if (!s || s == "") {
        cma = IR::MAU::ColorMapramAddress::NOT_SET;
    }
    for (int i = 0; i <= static_cast<int>(IR::MAU::ColorMapramAddress::NOT_SET); i++) {
        if (color_mapram_address_to_str[i] == s) {
            cma = static_cast<IR::MAU::ColorMapramAddress>(i); return true;
        }
    }
    return false;
}

static const char *selector_mode_to_str[] = {
    "FAIR", "RESILIENT", "SELECTOR_MODES"
};

std::ostream &operator<<(std::ostream &out, const IR::MAU::SelectorMode &t) {
    out << selector_mode_to_str[static_cast<int>(t)];
    return out;
}

bool operator>>(cstring s, IR::MAU::SelectorMode &t) {
    for (int i = 0; i <= static_cast<int>(IR::MAU::SelectorMode::SELECTOR_MODES); i++) {
        if (selector_mode_to_str[i] == s) {
            t = static_cast<IR::MAU::SelectorMode>(i); return true;
        }
    }
    return false;
}

static const char *always_run_to_str[] = {
    "NONE", "TABLE", "ACTION"
};

std::ostream& operator<<(std::ostream &out, const IR::MAU::AlwaysRun &ar) {
    out << always_run_to_str[static_cast<int>(ar)];
    return out;
}

bool operator>>(cstring s, IR::MAU::AlwaysRun &ar) {
    for (int i = 0; i < 3; i++) {
        if (always_run_to_str[i] == s) {
            ar = static_cast<IR::MAU::AlwaysRun>(i);
            return true;
        }
    }
    return false;
}

}  // end namespace MAU
}  // end namespace IR

namespace IR {
namespace BFN {

static const char *checksum_mode_to_str[] = {
    "VERIFY", "RESIDUAL", "CLOT"
};

std::ostream &operator<<(std::ostream &out, const IR::BFN::ChecksumMode &t) {
    out << checksum_mode_to_str[static_cast<int>(t)];
    return out;
}

bool operator>>(cstring s, IR::BFN::ChecksumMode &t) {
    for (int i = 0; i < 3; i++) {
        if (checksum_mode_to_str[i] == s) {
            t = static_cast<IR::BFN::ChecksumMode>(i); return true;
        }
    }
    return false;
}


static const char *parser_write_mode_to_str[] = {
    "SINGLE_WRITE", "BITWISE_OR", "CLEAR_ON_WRITE"
};

std::ostream &operator<<(std::ostream &out, const IR::BFN::ParserWriteMode &t) {
    out << parser_write_mode_to_str[static_cast<int>(t)];
    return out;
}

bool operator>>(cstring s, IR::BFN::ParserWriteMode &t) {
    for (int i = 0; i < 3; i++) {
        if (parser_write_mode_to_str[i] == s) {
            t = static_cast<IR::BFN::ParserWriteMode>(i); return true;
        }
    }
    return false;
}

}  // end namespace BFN
}  // end namespace IR
}  // end namespace P4
