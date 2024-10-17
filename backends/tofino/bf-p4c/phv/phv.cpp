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

#include "bf-p4c/phv/phv.h"

#include <cstring>
#include <iostream>
#include <sstream>

#include "lib/cstring.h"
#include "ir/ir.h"
#include "ir/json_loader.h"

namespace PHV {

Type::Type(Type::TypeEnum te) {
    switch (te) {
        case Type::B:  kind_ = Kind::normal;   size_ = Size::b8;  break;
        case Type::H:  kind_ = Kind::normal;   size_ = Size::b16; break;
        case Type::W:  kind_ = Kind::normal;   size_ = Size::b32; break;
        case Type::TB: kind_ = Kind::tagalong; size_ = Size::b8;  break;
        case Type::TH: kind_ = Kind::tagalong; size_ = Size::b16; break;
        case Type::TW: kind_ = Kind::tagalong; size_ = Size::b32; break;
        case Type::MB: kind_ = Kind::mocha;    size_ = Size::b8;  break;
        case Type::MH: kind_ = Kind::mocha;    size_ = Size::b16; break;
        case Type::MW: kind_ = Kind::mocha;    size_ = Size::b32; break;
        case Type::DB: kind_ = Kind::dark;     size_ = Size::b8;  break;
        case Type::DH: kind_ = Kind::dark;     size_ = Size::b16; break;
        case Type::DW: kind_ = Kind::dark;     size_ = Size::b32; break;
        default: BUG("Unknown PHV type"); }
}

Type::Type(const char* name, bool abort_if_invalid) {
    const char* n = name;

    switch (*n) {
        case 'T': kind_ = Kind::tagalong; n++; break;
        case 'M': kind_ = Kind::mocha;    n++; break;
        case 'D': kind_ = Kind::dark;     n++; break;
        default:  kind_ = Kind::normal; }

    switch (*n++) {
        case 'B': size_ = Size::b8;  break;
        case 'H': size_ = Size::b16; break;
        case 'W': size_ = Size::b32; break;
        default:
            size_ = Size::null;
            if (abort_if_invalid)
                BUG("Invalid PHV type '%s'", name); }

    if (*n && abort_if_invalid)
        BUG("Invalid PHV type '%s'", name);
}

unsigned Type::log2sz() const {
    switch (size_) {
       case Size::b8:   return 0;
       case Size::b16:  return 1;
       case Size::b32:  return 2;
       default: BUG("Called log2sz() on an invalid container");
    }
}

cstring Type::toString() const {
    std::stringstream tmp;
    tmp << *this;
    return tmp.str();
}

Container::Container(const char *name, bool abort_if_invalid) {
    const char *n = name + strcspn(name, "0123456789");
    type_ = Type(std::string(name, n - name).c_str(), abort_if_invalid);

    char *end = nullptr;
    auto v = strtol(n, &end, 10);
    index_ = v;
    if (end == n || *end || index_ != static_cast<unsigned long>(v)) {
        type_ = Type();
        if (abort_if_invalid)
            BUG("Invalid register '%s'", name); }
}

cstring Container::toString() const {
    std::stringstream tmp;
    tmp << *this;
    return tmp.str();
}

void Container::toJSON(P4::JSONGenerator& json) const {
    json << *this;
}

/* static */ Container Container::fromJSON(P4::JSONLoader& json) {
    if (auto* v = json.json->to<JsonString>())
        return Container(v->c_str());
    BUG("Couldn't decode JSON value to container");
    return Container();
}

cstring FieldUse::toString(unsigned dark) const {
        if (use_ == 0) return ""_cs;
        std::stringstream ss;
        bool checkLiveness = true;
        if (use_ & READ) {
            ss << (dark & READ ? "R" : "r");
            checkLiveness = false;
        }
        if (use_ & WRITE) {
            ss << (dark & WRITE ? "W" : "w");
            checkLiveness = false;
        }
        if (checkLiveness && (use_ & LIVE))
            ss << "~";
        return ss.str();
    }


bool LiveRange::is_disjoint(const LiveRange& other) const {
    const StageAndAccess& a = this->start;
    const StageAndAccess& b = this->end;
    const StageAndAccess& c = other.start;
    const StageAndAccess& d = other.end;

    // if any of the pairs is [xR, xR] pair, which represents an empty live range,
    // it is considered to be disjoint to any other live range.
    if ((a == b && a.second == PHV::FieldUse(PHV::FieldUse::READ)) ||
        (c == d && c.second == PHV::FieldUse(PHV::FieldUse::READ))) {
        return true;
    }
    // If the live ranges of current slice is [aA, bB] and that of other slice is [cC, dD], where
    // the small letters indicate stage and the capital letters indicate access type (read or
    // write).
    // The live ranges are disjoint only if:
    // ((a < c || (a == c && A < C)) && (b < c || (b == c && B < C))) ||
    // ((c < a || (c == a && C < A)) && (d < a || (d == a && D < A)))
    // TODO: this is compatible with physical live range mode. So even if there is a
    // much easier way to check disjoint in physical live range mode, we can keep using this.
    if ((((a.first < c.first) || (a.first == c.first && a.second < c.second)) &&
         ((b.first < c.first) || (b.first == c.first && b.second < c.second))) ||
        (((c.first < a.first) || (c.first == a.first && c.second < a.second)) &&
         ((d.first < a.first) || (d.first == a.first && d.second < a.second))))
        return true;
    return false;
}


void LiveRange::extend(const StageAndAccess& access) {
    if (access.first < start.first) {
        start = access;
    } else if (end.first < access.first) {
        end = access;
    }
    if (access.first == start.first) {
        start.second |= access.second;
    }
    if (access.first == end.first) {
        end.second |= access.second;
    }
}

std::ostream& operator<<(std::ostream& out, const PHV::Kind k) {
    switch (k) {
        case PHV::Kind::normal:   return out << "";
        case PHV::Kind::tagalong: return out << "T";
        case PHV::Kind::mocha:    return out << "M";
        case PHV::Kind::dark:     return out << "D";
        default:    BUG("Unknown PHV container kind");
    }
}

std::ostream& operator<<(std::ostream& out, const PHV::Size sz) {
    switch (sz) {
        case PHV::Size::b8:  return out << "B";
        case PHV::Size::b16: return out << "H";
        case PHV::Size::b32: return out << "W";
        default:             return out << "null";
    }
}

std::ostream& operator<<(std::ostream& out, PHV::Type t) {
    return out << t.kind() << t.size();
}

std::ostream& operator<<(std::ostream& out, const PHV::Container c) {
    return out << c.type() << c.index();
}

P4::JSONGenerator& operator<<(P4::JSONGenerator& out, const PHV::Container c) {
    return out << c.toString();
}

std::ostream& operator<<(std::ostream& out, ordered_set<const PHV::Container *>& c_set) {
    out << "{";
    for (auto &c : c_set) {
        out << *c << ",";
    }
    out << "}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const PHV::FieldUse u) {
    return out << u.toString();
}

std::ostream& operator<<(std::ostream& out, const StageAndAccess& s) {
    return out << s.first << s.second;
}

std::ostream& operator<<(std::ostream& out, const LiveRange& s) {
    return out << "[" << s.start << ", " << s.end << "]";
}

}  // namespace PHV
