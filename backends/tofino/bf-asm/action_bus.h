/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_ASM_ACTION_BUS_H_
#define BACKENDS_TOFINO_BF_ASM_ACTION_BUS_H_

#include <array>

#include "backends/tofino/bf-asm/tables.h"

// static struct MeterBus_t {} MeterBus;
struct MeterBus_t {};

struct ActionBusSource {
    enum {
        None,
        Field,
        HashDist,
        HashDistPair,
        RandomGen,
        TableOutput,
        TableColor,
        TableAddress,
        Ealu,
        XcmpData,
        NameRef,
        ColorRef,
        AddressRef
    } type;
    union {
        Table::Format::Field *field;
        HashDistribution *hd;
        struct {
            HashDistribution *hd1, *hd2;
        } hd_tuple;
        Table *table;
        Table::Ref *name_ref;
        RandomNumberGen rng;
        struct {
            short xcmp_group, xcmp_byte;
        } xcmp_data;
    };
    ActionBusSource() : type(None) { field = nullptr; }
    ActionBusSource(Table::Format::Field *f) : type(Field) {  // NOLINT(runtime/explicit)
        field = f;
    }
    ActionBusSource(HashDistribution *h) : type(HashDist) { hd = h; }  // NOLINT(runtime/explicit)
    ActionBusSource(HashDistribution *h1, HashDistribution *h2) : type(HashDistPair) {
        hd_tuple.hd1 = h1;
        hd_tuple.hd2 = h2;
    }
    ActionBusSource(Table *t,
                    TableOutputModifier m = TableOutputModifier::NONE)  // NOLINT(runtime/explicit)
        : type(TableOutput) {
        switch (m) {
            case TableOutputModifier::Color:
                type = TableColor;
                break;
            case TableOutputModifier::Address:
                type = TableAddress;
                break;
            default:
                break;
        }
        table = t;
    }
    ActionBusSource(Table::Ref *t,
                    TableOutputModifier m = TableOutputModifier::NONE)  // NOLINT(runtime/explicit)
        : type(NameRef) {
        switch (m) {
            case TableOutputModifier::Color:
                type = ColorRef;
                break;
            case TableOutputModifier::Address:
                type = AddressRef;
                break;
            default:
                break;
        }
        name_ref = t;
    }
    ActionBusSource(MeterBus_t,
                    TableOutputModifier m = TableOutputModifier::NONE)  // NOLINT(runtime/explicit)
        : type(NameRef) {
        switch (m) {
            case TableOutputModifier::Color:
                type = ColorRef;
                break;
            case TableOutputModifier::Address:
                type = AddressRef;
                break;
            default:
                break;
        }
        name_ref = nullptr;
    }
    ActionBusSource(RandomNumberGen r) : type(RandomGen) {  // NOLINT(runtime/explicit)
        field = nullptr;
        rng = r;
    }
    ActionBusSource(InputXbar::Group grp, int byte) : type(XcmpData) {
        BUG_CHECK(grp.type == InputXbar::Group::XCMP, "Not xcmp ixbar");
        field = nullptr;
        xcmp_data.xcmp_group = grp.index;
        xcmp_data.xcmp_byte = byte;
    }
    bool operator==(const ActionBusSource &a) const {
        if (type == XcmpData)
            return a.type == XcmpData && xcmp_data.xcmp_group == a.xcmp_data.xcmp_group &&
                   xcmp_data.xcmp_byte == a.xcmp_data.xcmp_byte;
        if (type == HashDistPair && hd_tuple.hd2 != a.hd_tuple.hd2) return false;
        return type == a.type && field == a.field;
    }
    bool operator<(const ActionBusSource &a) const {
        if (type != a.type) return type < a.type;
        switch (type) {
            case HashDistPair:
                return hd_tuple.hd1 == a.hd_tuple.hd1 ? hd_tuple.hd2 < a.hd_tuple.hd2
                                                      : hd_tuple.hd1 < a.hd_tuple.hd1;
            case XcmpData:
                return xcmp_data.xcmp_group == a.xcmp_data.xcmp_group
                           ? xcmp_data.xcmp_byte < a.xcmp_data.xcmp_byte
                           : xcmp_data.xcmp_group < a.xcmp_data.xcmp_group;
            default:
                return field < a.field;
        }
    }
    std::string name(Table *tbl) const;
    std::string toString(Table *tbl) const;
    friend std::ostream &operator<<(std::ostream &, const ActionBusSource &);
};

class ActionBus {
 protected:
    // Check two ActionBusSource refs to ensure that they are compatible (can be at the same
    // location on the aciton bus -- basically the same data)
    static bool compatible(const ActionBusSource &a, unsigned a_off, const ActionBusSource &b,
                           unsigned b_off);
    struct Slot {
        std::string name;
        unsigned byte, size;  // size in bits
        ordered_map<ActionBusSource, unsigned> data;
        // offset in the specified source is in this slot -- corresponding bytes for different
        // action data formats will go into the same slot.
        Slot(std::string n, unsigned b, unsigned s) : name(n), byte(b), size(s) {}
        Slot(std::string n, unsigned b, unsigned s, ActionBusSource src, unsigned off)
            : name(n), byte(b), size(s) {
            data.emplace(src, off);
        }
        unsigned lo(Table *tbl) const;  // low bit on the action data bus
        bool is_table_output() const {
            for (auto &d : data) {
                BUG_CHECK(d.first.type != ActionBusSource::NameRef, "Unexpected name ref");
                if (d.first.type == ActionBusSource::TableOutput) return true;
            }
            return false;
        }
    };
    friend std::ostream &operator<<(std::ostream &, const Slot &);
    friend std::ostream &operator<<(std::ostream &, const ActionBus &);
    ordered_map<unsigned, Slot> by_byte;
    ordered_map<ActionBusSource, ordered_map<unsigned, unsigned>> need_place;
    // bytes from the given sources are needed on the action bus -- the pairs in the map
    // are (offset,use) where offset is offset in bits, and use is a bitset of the needed
    // uses (bit index == log2 of the access size in bytes)

    std::vector<std::array<unsigned, ACTION_HV_XBAR_SLICES>> action_hv_slice_use;
    // which bytes of input to the ixbar are used in each action_hv_xbar slice, for each
    // 128-bit slice of the action bus.
    bitvec byte_use;  // bytes on the action data (input) bus or immediate bus in use
                      // for wide action tables, this may be >16 bytes...

    void setup_slot(int lineno, Table *tbl, const char *name, unsigned idx, ActionBusSource src,
                    unsigned sz, unsigned off);

    int find_free(Table *tbl, unsigned min, unsigned max, unsigned step, unsigned lobyte,
                  unsigned bytes);
    int find_merge(Table *tbl, int offset, int bytes, int use);
    bool check_atcam_sharing(Table *tbl1, Table *tbl2);
    bool check_slot_sharing(ActionBus::Slot &slot, bitvec &action_bus);

    ActionBus() : lineno(-1) {}
    ActionBus(Table *, VECTOR(pair_t) &);

 public:
    int lineno;
    static std::unique_ptr<ActionBus> create();
    static std::unique_ptr<ActionBus> create(Table *, VECTOR(pair_t) &);

    void pass1(Table *tbl);
    void pass2(Table *tbl) {}
    void pass3(Table *tbl);
    template <class REGS>
    void write_immed_regs(REGS &regs, Table *tbl);
    template <class REGS>
    void write_action_regs(REGS &regs, Table *tbl, int homerow, unsigned action_slice);

    void do_alloc(Table *tbl, ActionBusSource src, unsigned use, int lobyte, int bytes,
                  unsigned offset);
    static const unsigned size_masks[8];
    virtual void alloc_field(Table *, ActionBusSource src, unsigned offset, unsigned sizes_needed);
    void need_alloc(Table *tbl, const ActionBusSource &src, unsigned lo, unsigned hi,
                    unsigned size);
    void need_alloc(Table *tbl, Table *attached, TableOutputModifier mod, unsigned lo, unsigned hi,
                    unsigned size) {
        need_alloc(tbl, ActionBusSource(attached, mod), lo, hi, size);
    }

    int find(const char *name, TableOutputModifier mod, int lo, int hi, int size, int *len = 0);
    int find(const char *name, int lo, int hi, int size, int *len = 0) {
        return find(name, TableOutputModifier::NONE, lo, hi, size, len);
    }
    int find(const std::string &name, TableOutputModifier mod, int lo, int hi, int size,
             int *len = 0) {
        return find(name.c_str(), mod, lo, hi, size, len);
    }
    int find(const std::string &name, int lo, int hi, int size, int *len = 0) {
        return find(name.c_str(), lo, hi, size, len);
    }
    int find(const ActionBusSource &src, int lo, int hi, int size, int pos = -1, int *len = 0);
    int find(Table *attached, TableOutputModifier mod, int lo, int hi, int size, int *len = 0) {
        return find(ActionBusSource(attached, mod), lo, hi, size, -1, len);
    }
    static int find(Stage *stage, ActionBusSource src, int lo, int hi, int size, int *len = 0);
    unsigned size() {
        unsigned rv = 0;
        for (auto &slot : by_byte) rv += slot.second.size;
        return rv;
    }
    auto slots() const { return Values(by_byte); }
};

#endif /* BACKENDS_TOFINO_BF_ASM_ACTION_BUS_H_ */
