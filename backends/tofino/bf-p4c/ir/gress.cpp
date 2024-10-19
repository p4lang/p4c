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

#include "gress.h"

#include <lib/cstring.h>
#include <lib/exceptions.h>

#include <boost/optional/optional_io.hpp>

using namespace P4::literals;

cstring toString(gress_t gress) {
    switch (gress) {
        case INGRESS:
            return "ingress"_cs;
        case EGRESS:
            return "egress"_cs;
        case GHOST:
            return "ghost"_cs;
        default:
            BUG("Unexpected *gress value");
    }
}

cstring toSymbol(gress_t gress) {
    switch (gress) {
        case INGRESS:
            return "I"_cs;
        case EGRESS:
            return "E"_cs;
        case GHOST:
            return "G"_cs;
        default:
            BUG("Unexpected *gress value");
    }
}

cstring createThreadName(gress_t gress, cstring name) { return toString(gress) + "::" + name; }

cstring stripThreadPrefix(cstring name) {
    if (auto *pos = name.find("::")) {
        return cstring(pos + 2);
    }
    return name;
}

std::ostream &operator<<(std::ostream &out, gress_t gress) { return out << toString(gress); }

std::ostream &operator<<(std::ostream &out, std::optional<gress_t> gress) {
    if (gress)
        out << *gress;
    else
        out << "none";
    return out;
}

bool operator>>(cstring s, gress_t &gressOut) {
    if (s == "ingress")
        gressOut = INGRESS;
    else if (s == "egress")
        gressOut = EGRESS;
    else if (s == "ghost")
        gressOut = GHOST;
    else
        return false;
    return true;
}
