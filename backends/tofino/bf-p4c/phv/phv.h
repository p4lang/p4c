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

#ifndef BF_P4C_PHV_PHV_H_
#define BF_P4C_PHV_PHV_H_

#include <iosfwd>
#include <limits>

#include <boost/functional/hash.hpp>

#include "lib/exceptions.h"
#include "lib/ordered_set.h"

namespace P4 {
class bitvec;
class cstring;
class JSONGenerator;
class JSONLoader;
}  // namespace P4

using namespace P4;
using namespace P4::literals;

namespace PHV {

/// all possible PHV container kinds in BFN devices
/// The values here are used to define operator< on Kinds.
enum class Kind : unsigned short { tagalong = 0, dark = 1, mocha = 2, normal = 3 };

const Kind KINDS[] = {Kind::tagalong, Kind::dark, Kind::mocha, Kind::normal};

const std::map<Kind, cstring> STR_OF_KIND = {{Kind::tagalong, "tagalong"_cs},
                                             {Kind::dark, "dark"_cs},
                                             {Kind::mocha, "mocha"_cs},
                                             {Kind::normal, "normal"_cs}};

// all possible contexts a PHV container can participate in
enum class Context : unsigned short {
    parde = 0,
    ixbar = 1,
    vliw = 2,
    vliw_set = 3  // Whole container move only
};

inline std::vector<Context> all_contexts(Kind kind) {
    switch (kind) {
        case PHV::Kind::normal:
            return {Context::parde, Context::ixbar, Context::vliw, Context::vliw_set};

        case PHV::Kind::tagalong:
            return {Context::parde};

        case PHV::Kind::mocha:
            return {Context::parde, Context::ixbar, Context::vliw_set};  // move only

        case PHV::Kind::dark:
            return {Context::vliw_set};  // move only

        default:
            BUG("Unknown PHV container kind");
    }
}

/** Provides a total ordering over PHV container kinds, denoting a refinement of the ordering
 * induced by subset inclusion over the set of capabilities each kind supports.  For example,
 * `tagalong < normal`, because tagalong containers don't support reads/writes in the MAU, whereas
 * normal containers do.
 */
// TODO: This was previously a partial order. Turned into a total order so that things behave
// properly when Kind is used as a key in std::map or an element in std::set. It's hard to search
// for all the places where this operator is used, so I'm not confident that this won't break
// anything.
//
// Previous ordering: normal > mocha > dark, mocha > tagalong. Deep had a comment that dark and
// tagalong don't occur together, so an ordering between them didn't need to be established, but
// the same could be said for tagalong and mocha.
//
// Current ordering: normal > mocha > dark > tagalong.
inline bool operator<(Kind left, Kind right) {
    // Unfortunately, using the all_contexts map to define this inequality causes a massive slowdown
    // (>2x) in compilation times.
    // TODO: Figure out a way to use the all_contexts map to define this inequality.

    return (size_t)left < (size_t)right;
}

inline bool operator<=(Kind left, Kind right) { return left == right || left < right; }

inline bool operator>(Kind left, Kind right) { return right < left; }

inline bool operator>=(Kind left, Kind right) { return left == right || left > right; }

/// all possible PHV container sizes in BFN devices
enum class Size : unsigned short { null = 0, b8 = 8, b16 = 16, b32 = 32 };

const Size SIZES[] = {Size::null, Size::b8, Size::b16, Size::b32};

class Type {
    Kind kind_;
    Size size_;

 public:
    enum TypeEnum {  // all possible PHV container types in BFN devices
        B,           //     8-b  normal
        H,           //     16-b  |
        W,           //     32-b _|
        TB,          //     8-b  tagalong
        TH,          //     16-b  |
        TW,          //     32-b _|
        MB,          //     8-b  mocha
        MH,          //     16-b  |
        MW,          //     32-b _|
        DB,          //     8-b  dark
        DH,          //     16-b  |
        DW           //     32-b _|
    };

    Type() : kind_(Kind::normal), size_(Size::null) {}
    Type(Kind k, Size s) : kind_(k), size_(s) {}
    Type(const Type &t) : kind_(t.kind_), size_(t.size_) {}

    Type(TypeEnum te);                                     // NOLINT(runtime/explicit)
    Type(const char *name, bool abort_if_invalid = true);  // NOLINT(runtime/explicit)

    unsigned log2sz() const;
    Kind kind() const { return kind_; }
    Size size() const { return size_; }

    Type &operator=(const Type &t) {
        kind_ = t.kind_;
        size_ = t.size_;
        return *this;
    }

    bool operator==(Type c) const { return (kind_ == c.kind_) && (size_ == c.size_); }

    bool operator!=(Type c) const { return !(*this == c); }

    bool operator<(Type c) const {
        if (kind_ < c.kind_) return true;
        if (c.kind_ < kind_) return false;
        if (size_ < c.size_) return true;
        if (size_ > c.size_) return false;
        return false;
    }

    friend size_t hash_value(const Type &c) {
        size_t h = 0;
        boost::hash_combine(h, c.kind_);
        boost::hash_combine(h, c.size_);
        return h;
    }

    bool valid() const { return size_ != Size::null; }

    /// @return a string representation of this container type.
    cstring toString() const;
};

class Container {
    static constexpr unsigned MAX_INDEX = std::numeric_limits<unsigned>::max();

    Type type_;
    unsigned index_;

    Container(Kind k, Size s, unsigned i) : type_(k, s), index_(i) {}

 public:
    /// Construct an empty container. Most operations aren't defined on empty
    /// containers. (Use `operator bool()` to check if a container is empty.)
    Container() : type_(), index_(0) {}

    /// Construct a container from @name - e.g., "B0" for container B0.
    Container(const char *name, bool abort_if_invalid = true);  // NOLINT(runtime/explicit)

    /// Construct a container from @p kind and @p index - e.g., (Kind::B, 0) for
    /// container B0.
    Container(PHV::Type t, unsigned index) : type_(t), index_(index) {}

    size_t size() const { return size_t(type_.size()); }
    unsigned log2sz() const { return type_.log2sz(); }
    size_t msb() const { return size() - 1; }
    size_t lsb() const { return 0; }

    PHV::Type type() const { return type_; }
    unsigned index() const { return index_; }

    bool is(PHV::Kind k) const { return k == type_.kind(); }
    bool is(PHV::Size sz) const { return sz == type_.size(); }

    /// @return true if this container is nonempty (i.e., refers to an actual
    /// PHV container).
    explicit operator bool() const { return type_.valid(); }

    Container operator++() {
        if (index_ != MAX_INDEX) ++index_;
        return *this;
    }
    Container operator++(int) {
        Container rv = *this;
        ++*this;
        return rv;
    }
    bool operator==(Container c) const { return type_ == c.type_ && index_ == c.index_; }
    bool operator!=(Container c) const { return !(*this == c); }
    bool operator<(Container c) const {
        if (type_ < c.type_) return true;
        if (c.type_ < type_) return false;
        if (index_ < c.index_) return true;
        if (c.index_ < index_) return false;
        return false;
    }

    friend size_t hash_value(const Container &c) {
        size_t h = 0;
        boost::hash_combine(h, c.type_);
        boost::hash_combine(h, c.index_);
        return h;
    }

    /// JSON serialization/deserialization.
    void toJSON(P4::JSONGenerator &json) const;
    static Container fromJSON(P4::JSONLoader &json);

    /// @return a string representation of this container.
    cstring toString() const;
};

/** This class describes the state of the use of a PHV container. In any given unit, the PHV may be
 * read, written, both read and written, or merely be live.
 */
class FieldUse {
    unsigned use_;

 public:
    enum use_t { READ = 1, WRITE = 2, READWRITE = READ | WRITE, LIVE = 4 };

    // Construct an empty use, which is neither read, write, or live.
    FieldUse() : use_(0) {}

    // Construct a FieldUse object, given a specific kind of use.
    explicit FieldUse(unsigned u) {
        BUG_CHECK(u == READ || u == WRITE || u == READWRITE || u == LIVE,
                  "Invalid value %1% used to create a FieldUse object. Valid values are "
                  "1: READ, 2: WRITE, 3: READWRITE, 4: LIVE",
                  u);
        use_ = u;
    }

    bool isRead() const { return use_ & READ; }
    bool isOnlyReadAndNotLive() const { return use_ == READ; }
    bool isOnlyWriteAndNotLive() const { return use_ == WRITE; }
    bool isWrite() const { return use_ & WRITE; }
    bool isLive() const { return use_ & LIVE; }
    bool isReadWrite() const { return use_ & READWRITE; }
    bool isReadAndWrite() const { return use_ == READWRITE; }

    bool operator==(FieldUse u) const { return use_ == u.use_; }
    bool operator!=(FieldUse u) const { return !(*this == u); }
    bool operator<(FieldUse u) const { return use_ < u.use_; }
    bool operator>(FieldUse u) const { return use_ > u.use_; }
    bool operator<=(FieldUse u) const { return use_ <= u.use_; }
    bool operator>=(FieldUse u) const { return use_ >= u.use_; }
    explicit operator bool() const { return use_ == 0; }

    FieldUse operator|(const FieldUse &u) const {
        FieldUse ru;
        ru.use_ = use_ | u.use_;
        BUG_CHECK(ru.use_ <= (READWRITE | LIVE), "invalid use of FieldUse %1%", ru.use_);
        return ru;
    }

    FieldUse &operator|=(const FieldUse &u) {
        use_ |= u.use_;
        BUG_CHECK(use_ <= (READWRITE | LIVE), "invalid use of FieldUse%1%", use_);
        return *this;
    }

    FieldUse operator&(const FieldUse &u) const {
        FieldUse ru;
        ru.use_ = use_ & u.use_;
        BUG_CHECK(ru.use_ <= (READWRITE | LIVE), "invalid use of FieldUse%1%", ru.use_);
        return ru;
    }

    cstring toString(unsigned dark = 0) const;
};

// Pair of stage number and type of access for the field.
using StageAndAccess = std::pair<int, FieldUse>;

// Pair of two accesses.
struct LiveRange {
    StageAndAccess start;
    StageAndAccess end;
    LiveRange(const StageAndAccess &start, const StageAndAccess &end) : start(start), end(end) {}
    bool is_disjoint(const LiveRange &other) const;
    // extend live range to include @p access.
    void extend(const StageAndAccess &access);
    bool operator<(const LiveRange &other) const {
        if (start.first != other.start.first)
            return start.first < other.start.first;
        else
            return end.first < other.end.first;
    }
    bool operator==(const LiveRange &other) const {
        return start == other.start && end == other.end;
    }
};

std::ostream &operator<<(std::ostream &out, const PHV::Kind k);
std::ostream &operator<<(std::ostream &out, const PHV::Size sz);
std::ostream &operator<<(std::ostream &out, const PHV::Type t);
std::ostream &operator<<(std::ostream &out, const PHV::Container c);
std::ostream &operator<<(std::ostream &out, ordered_set<const PHV::Container *> &c_set);
std::ostream &operator<<(std::ostream &out, const PHV::FieldUse u);
std::ostream &operator<<(std::ostream &out, const StageAndAccess &s);
std::ostream &operator<<(std::ostream &out, const LiveRange &s);

P4::JSONGenerator &operator<<(P4::JSONGenerator &out, const PHV::Container c);
}  // namespace PHV

#endif /* BF_P4C_PHV_PHV_H_ */
