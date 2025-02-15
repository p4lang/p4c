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

#include "action_bus.h"

#include "backends/tofino/bf-asm/config.h"
#include "backends/tofino/bf-asm/stage.h"
#include "lib/hex.h"
#include "misc.h"

static MeterBus_t MeterBus;

std::ostream &operator<<(std::ostream &out, const ActionBusSource &src) {
    const char *sep = "";
    switch (src.type) {
        case ActionBusSource::None:
            out << "None";
            break;
        case ActionBusSource::Field:
            out << "Field(";
            for (auto &range : src.field->bits) {
                out << sep << range.lo << ".." << range.hi;
                sep = ", ";
            }
            out << ")";
            if (src.field->fmt && src.field->fmt->tbl)
                out << " " << src.field->fmt->tbl->find_field(src.field);
            break;
        case ActionBusSource::HashDist:
            out << "HashDist(" << src.hd->hash_group << ", " << src.hd->id << ")";
            break;
        case ActionBusSource::HashDistPair:
            out << "HashDistPair([" << src.hd1->hash_group << ", " << src.hd1->id << "]," << "["
                << src.hd2->hash_group << ", " << src.hd2->id << "])";
            break;
        case ActionBusSource::RandomGen:
            out << "rng " << src.rng.unit;
            break;
        case ActionBusSource::TableOutput:
            out << "TableOutput(" << (src.table ? src.table->name() : "0") << ")";
            break;
        case ActionBusSource::TableColor:
            out << "TableColor(" << (src.table ? src.table->name() : "0") << ")";
            break;
        case ActionBusSource::TableAddress:
            out << "TableAddress(" << (src.table ? src.table->name() : "0") << ")";
            break;
        case ActionBusSource::Ealu:
            out << "EALU";
            break;
        case ActionBusSource::XcmpData:
            out << "XCMP(" << src.xcmp_group << ":" << src.xcmp_byte << ")";
            break;
        case ActionBusSource::NameRef:
            out << "NameRef(" << (src.name_ref ? src.name_ref->name : "0") << ")";
            break;
        case ActionBusSource::ColorRef:
            out << "ColorRef(" << (src.name_ref ? src.name_ref->name : "0") << ")";
            break;
        case ActionBusSource::AddressRef:
            out << "AddressRef(" << (src.name_ref ? src.name_ref->name : "0") << ")";
            break;
        default:
            out << "<invalid type 0x" << hex(src.type) << ">";
            break;
    }
    return out;
}

/* identifes which bytes on the action bus are tied together in the hv_xbar input,
 * so must be routed together.  The second table here is basically just bitcount of
 * masks in the first table. */
static std::array<std::array<unsigned, 16>, ACTION_HV_XBAR_SLICES> action_hv_slice_byte_groups = {{
    {0x3, 0x3, 0xc, 0xc, 0xf0, 0xf0, 0xf0, 0xf0, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00,
     0xff00, 0xff00},
    {0xf, 0xf, 0xf, 0xf, 0xf0, 0xf0, 0xf0, 0xf0, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00,
     0xff00, 0xff00},
    {0xf, 0xf, 0xf, 0xf, 0xf0, 0xf0, 0xf0, 0xf0, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00,
     0xff00, 0xff00},
    {0xf, 0xf, 0xf, 0xf, 0xf0, 0xf0, 0xf0, 0xf0, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00,
     0xff00, 0xff00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00,
     0xff00, 0xff00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00,
     0xff00, 0xff00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00,
     0xff00, 0xff00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00,
     0xff00, 0xff00},
}};

static std::array<std::array<int, 16>, ACTION_HV_XBAR_SLICES> action_hv_slice_group_align = {
    {{2, 2, 2, 2, 4, 4, 4, 4, 8, 8, 8, 8, 8, 8, 8, 8},
     {4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 8, 8, 8, 8, 8, 8},
     {4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 8, 8, 8, 8, 8, 8},
     {4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 8, 8, 8, 8, 8, 8},
     {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
     {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
     {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
     {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8}}};

ActionBus::ActionBus(Table *tbl, VECTOR(pair_t) & data) {
    lineno = data.size ? data[0].key.lineno : -1;
    for (auto &kv : data) {
        if (!CHECKTYPE2(kv.key, tINT, tRANGE)) continue;
        unsigned idx = kv.key.type == tRANGE ? kv.key.range.lo : kv.key.i;
        if (!CHECKTYPE2M(kv.value, tSTR, tCMD, "field name or slice")) continue;
        const char *name = kv.value.s;
        value_t *name_ref = &kv.value;
        unsigned off = 0, sz = 0;
        if (kv.value.type == tCMD) {
            BUG_CHECK(kv.value.vec.size > 0 && kv.value[0].type == tSTR);
            if (kv.value == "hash_dist" || kv.value == "rng") {
                if (!PCHECKTYPE(kv.value.vec.size > 1, kv.value[1], tINT)) continue;
                name = kv.value[0].s;
                name_ref = nullptr;
            } else {
                if (!PCHECKTYPE2M(kv.value.vec.size == 2, kv.value[1], tRANGE, tSTR,
                                  "field name or slice"))
                    continue;
                // if ((kv.value[1].range.lo & 7) != 0 || (kv.value[1].range.hi & 7) != 7) {
                //     error(kv.value.lineno, "Slice must be byte slice");
                //     continue; }
                name = kv.value[0].s;
                name_ref = &kv.value[0];
                if (kv.value[1].type == tRANGE) {
                    off = kv.value[1].range.lo;
                    sz = kv.value[1].range.hi - kv.value[1].range.lo + 1;
                } else if (kv.value[1] != "color") {
                    error(kv.value[1].lineno, "unexpected %s", kv.value[1].s);
                }
            }
        }
        Table::Format::Field *f = tbl->lookup_field(name, "*");
        ActionBusSource src;
        const char *p = name - 1;
        while (!f && (p = strchr(p + 1, '.')))
            f = tbl->lookup_field(p + 1, std::string(name, p - name));
        if (!f) {
            if (tbl->table_type() == Table::ACTION) {
                error(kv.value.lineno, "No field %s in format", name);
                continue;
            } else if (kv.value == "meter") {
                src = ActionBusSource(MeterBus);
                if (kv.value.type == tCMD) {
                    if (kv.value[1] == "color") {
                        src.type = ActionBusSource::ColorRef;
                        if (!sz) off = 24, sz = 8;
                    } else if (kv.value[1] == "address") {
                        src.type = ActionBusSource::AddressRef;
                    }
                }
            } else if (kv.value.type == tCMD && kv.value == "hash_dist") {
                if (auto hd = tbl->find_hash_dist(kv.value[1].i)) {
                    src = ActionBusSource(hd);
                } else {
                    error(kv.value.lineno, "No hash_dist %" PRId64 " in table %s", kv.value[1].i,
                          tbl->name());
                    continue;
                }
                sz = 16;
                for (int i = 2; i < kv.value.vec.size; ++i) {
                    if (kv.value[i] == "lo" || kv.value[i] == "low") {
                        src.hd->xbar_use |= HashDistribution::IMMEDIATE_LOW;
                    } else if (kv.value[i] == "hi" || kv.value[i] == "high") {
                        src.hd->xbar_use |= HashDistribution::IMMEDIATE_HIGH;
                        off += 16;
                    } else if (kv.value[i].type == tINT) {
                        if (auto hd_hi = tbl->find_hash_dist(kv.value[i].i)) {
                            src.hd->xbar_use |= HashDistribution::IMMEDIATE_LOW;
                            hd_hi->xbar_use |= HashDistribution::IMMEDIATE_HIGH;
                            setup_slot(kv.value.lineno, tbl, name, idx + 2, ActionBusSource(hd_hi),
                                       16, 16);
                            setup_slot(kv.value.lineno, tbl, name, idx,
                                       ActionBusSource(src.hd, hd_hi), 32, 0);
                        }
                    } else if (kv.value[i].type == tRANGE) {
                        if ((kv.value[i].range.lo & 7) != 0 || (kv.value[i].range.hi & 7) != 7)
                            error(kv.value.lineno, "Slice must be byte slice");
                        off += kv.value[i].range.lo;
                        sz = kv.value[i].range.hi - kv.value[i].range.lo + 1;
                    } else {
                        error(kv.value[i].lineno, "Unexpected hash_dist %s",
                              value_desc(kv.value[i]));
                        break;
                    }
                }
            } else if (kv.value.type == tCMD && kv.value == "rng") {
                src = ActionBusSource(RandomNumberGen(kv.value[1].i));
                if (kv.value.vec.size > 2 && CHECKTYPE(kv.value[2], tRANGE)) {
                    off = kv.value[2].range.lo;
                    sz = kv.value[2].range.hi + 1 - off;
                }
            } else if (name_ref) {
                src = ActionBusSource(new Table::Ref(*name_ref));
                if (kv.value.type == tCMD) {
                    if (kv.value[1] == "color") {
                        src.type = ActionBusSource::ColorRef;
                        if (!sz) off = 24, sz = 8;
                    } else if (kv.value[1] == "address") {
                        src.type = ActionBusSource::AddressRef;
                    }
                }
            } else if (tbl->format) {
                error(kv.value.lineno, "No field %s in format", name);
                continue;
            }
        } else {
            src = ActionBusSource(f);
            if (!sz) sz = f->size;
            if (off + sz > f->size)
                error(kv.value.lineno, "Invalid slice of %d bit field %s", f->size, name);
        }
        if (kv.key.type == tRANGE) {
            unsigned size = (kv.key.range.hi - idx + 1) * 8;
            // Make slot size (sz) same as no. of bytes allocated on action bus.
            if (size > sz) sz = size;
        } else if (!sz) {
            sz = idx < ACTION_DATA_8B_SLOTS                               ? 8
                 : idx < ACTION_DATA_8B_SLOTS + 2 * ACTION_DATA_16B_SLOTS ? 16
                                                                          : 32;
        }
        setup_slot(kv.key.lineno, tbl, name, idx, src, sz, off);
        tbl->apply_to_field(
            name, [](Table::Format::Field *f) { f->flags |= Table::Format::Field::USED_IMMED; });
        if (f) {
            auto &slot = by_byte.at(idx);
            tbl->apply_to_field(name, [&slot, tbl, off](Table::Format::Field *f) {
                ActionBusSource src(f);
                if (slot.data.emplace(src, off).second) {
                    LOG4("        data += " << src.toString(tbl) << " off=" << off);
                }
            });
        }
    }
}

std::unique_ptr<ActionBus> ActionBus::create() {
    return std::unique_ptr<ActionBus>(new ActionBus());
}

std::unique_ptr<ActionBus> ActionBus::create(Table *tbl, VECTOR(pair_t) & data) {
    return std::unique_ptr<ActionBus>(new ActionBus(tbl, data));
}

void ActionBus::setup_slot(int lineno, Table *tbl, const char *name, unsigned idx,
                           ActionBusSource src, unsigned sz, unsigned off) {
    if (idx >= ACTION_DATA_BUS_BYTES) {
        error(lineno, "Action bus index out of range");
        return;
    }
    if (by_byte.count(idx)) {
        auto &slot = by_byte.at(idx);
        if (sz > slot.size) {
            slot.name = name;
            slot.size = sz;
        }
        slot.data.emplace(src, off);
        LOG4("ActionBus::ActionBus: " << idx << ": " << name << " sz=" << sz
                                      << " data += " << src.toString(tbl) << " off=" << off);
    } else {
        by_byte.emplace(idx, Slot(name, idx, sz, src, off));
        LOG4("ActionBus::ActionBus: " << idx << ": " << name << " sz=" << sz
                                      << " data = " << src.toString(tbl) << " off=" << off);
    }
}

unsigned ActionBus::Slot::lo(Table *tbl) const {
    int rv = -1;
    for (auto &src : data) {
        int off = src.second;
        if (src.first.type == ActionBusSource::Field) off += src.first.field->immed_bit(0);
        BUG_CHECK(rv < 0 || rv == off);
        rv = off;
    }
    BUG_CHECK(rv >= 0);
    return rv;
}

bool ActionBus::compatible(const ActionBusSource &a, unsigned a_off, const ActionBusSource &b,
                           unsigned b_off) {
    if ((a.type == ActionBusSource::HashDist) && (b.type == ActionBusSource::HashDistPair)) {
        return ((compatible(a, a_off, ActionBusSource(b.hd1), b_off)) ||
                (compatible(a, a_off, ActionBusSource(b.hd2), b_off + 16)));
    } else if ((a.type == ActionBusSource::HashDistPair) && (b.type == ActionBusSource::HashDist)) {
        return ((compatible(ActionBusSource(a.hd1), a_off, b, b_off)) ||
                (compatible(ActionBusSource(a.hd2), a_off + 16, b, b_off)));
    }
    if (a.type != b.type) return false;
    switch (a.type) {
        case ActionBusSource::Field:
            // corresponding fields in different groups are compatible even though they
            // are at different locations.  Table::Format::pass1 checks that
            if (a.field->by_group == b.field->by_group) return true;
            return a.field->bit(a_off) == b.field->bit(b_off);
        case ActionBusSource::HashDist:
            return a.hd->hash_group == b.hd->hash_group && a.hd->id == b.hd->id && a_off == b_off;
        case ActionBusSource::HashDistPair:
            return ((a.hd1->hash_group == b.hd1->hash_group && a.hd1->id == b.hd1->id) &&
                    (a_off == b_off) &&
                    (a.hd2->hash_group == b.hd2->hash_group && a.hd2->id == b.hd2->id));
        case ActionBusSource::TableOutput:
            return a.table == b.table;
        default:
            return false;
    }
}

void ActionBus::pass1(Table *tbl) {
    bool is_immed_data = dynamic_cast<MatchTable *>(tbl) != nullptr;
    LOG1("ActionBus::pass1(" << tbl->name() << ")" << (is_immed_data ? " [immed]" : ""));
    if (lineno < 0)
        lineno = tbl->format && tbl->format->lineno >= 0 ? tbl->format->lineno : tbl->lineno;
    Slot *use[ACTION_DATA_BUS_SLOTS] = {0};
    for (auto &slot : Values(by_byte)) {
        for (auto it = slot.data.begin(); it != slot.data.end();) {
            if (it->first.type >= ActionBusSource::NameRef &&
                it->first.type <= ActionBusSource::AddressRef) {
                // Remove all NameRef and replace with TableOutputs or Fields
                // ColorRef turns into TableColor,  AddressRef into TableAddress
                if (it->first.name_ref) {
                    bool ok = false;
                    if (*it->first.name_ref) {
                        ActionBusSource src(*it->first.name_ref);
                        switch (it->first.type) {
                            case ActionBusSource::NameRef:
                                src.table->set_output_used();
                                break;
                            case ActionBusSource::ColorRef:
                                src.type = ActionBusSource::TableColor;
                                src.table->set_color_used();
                                break;
                            case ActionBusSource::AddressRef:
                                src.type = ActionBusSource::TableAddress;
                                src.table->set_address_used();
                                break;
                            default:
                                BUG();
                        }
                        slot.data[src] = it->second;
                        ok = true;
                    } else if (tbl->actions) {
                        Table::Format::Field *found_field = nullptr;
                        Table::Actions::Action *found_act = nullptr;
                        for (auto &act : *tbl->actions) {
                            int lo = -1, hi = -1;
                            auto name = act.alias_lookup(it->first.name_ref->lineno,
                                                         it->first.name_ref->name, lo, hi);
                            if (auto *field = tbl->lookup_field(name, act.name)) {
                                if (found_field) {
                                    if (field != found_field ||
                                        slot.data.at(ActionBusSource(field)) != it->second + lo)
                                        error(it->first.name_ref->lineno,
                                              "%s has incompatible "
                                              "aliases in actions %s and %s",
                                              it->first.name_ref->name.c_str(),
                                              found_act->name.c_str(), act.name.c_str());
                                } else {
                                    found_act = &act;
                                    found_field = field;
                                    slot.data[ActionBusSource(field)] = it->second + lo;
                                    ok = true;
                                }
                            }
                        }
                    }
                    if (!ok)
                        error(it->first.name_ref->lineno, "No format field or table named %s",
                              it->first.name_ref->name.c_str());
                } else {
                    auto att = tbl->get_attached();
                    if (!att || att->meters.empty()) {
                        error(lineno, "No meter table attached to %s", tbl->name());
                    } else if (att->meters.size() > 1) {
                        error(lineno, "Multiple meter tables attached to %s", tbl->name());
                    } else {
                        ActionBusSource src(att->meters.at(0));
                        switch (it->first.type) {
                            case ActionBusSource::NameRef:
                                src.table->set_output_used();
                                break;
                            case ActionBusSource::ColorRef:
                                src.type = ActionBusSource::TableColor;
                                src.table->set_color_used();
                                break;
                            case ActionBusSource::AddressRef:
                                src.type = ActionBusSource::TableAddress;
                                src.table->set_address_used();
                                break;
                            default:
                                BUG();
                        }
                        slot.data[src] = it->second;
                    }
                }
                it = slot.data.erase(it);
            } else {
                if (it->first.type == ActionBusSource::TableColor)
                    it->first.table->set_color_used();
                if (it->first.type == ActionBusSource::TableOutput)
                    it->first.table->set_output_used();
                ++it;
            }
        }
        if (error_count > 0) continue;
        auto first = slot.data.begin();
        if (first != slot.data.end()) {
            for (auto it = next(first); it != slot.data.end(); ++it) {
                if (!compatible(first->first, first->second, it->first, it->second))
                    error(lineno, "Incompatible action bus entries at offset %d", slot.byte);
            }
        }
        int slotno = Stage::action_bus_slot_map[slot.byte];
        for (unsigned byte = slot.byte; byte < slot.byte + slot.size / 8U;
             byte += Stage::action_bus_slot_size[slotno++] / 8U) {
            if (slotno >= ACTION_DATA_BUS_SLOTS) {
                error(lineno, "%s extends past the end of the actions bus", slot.name.c_str());
                break;
            }
            if (auto tbl_in_slot = tbl->stage->action_bus_use[slotno]) {
                if (tbl_in_slot != tbl) {
                    if (!(check_atcam_sharing(tbl, tbl_in_slot) ||
                          check_slot_sharing(slot, tbl->stage->action_bus_use_bit_mask)))
                        warning(lineno, "Action bus byte %d set in table %s and table %s", byte,
                                tbl->name(), tbl->stage->action_bus_use[slotno]->name());
                }
            } else {
                tbl->stage->action_bus_use[slotno] = tbl;
                // Set a per-byte mask on the action bus bytes to indicate which
                // bits in bytes are being used. A slot can be shared among
                // tables which dont overlap any bits. The code assumes the
                // action bus allocation is byte aligned (and sets the mask to
                // 0xF), while this could ideally be not the case. In that
                // event, the mask must be set accordingly. This will require
                // additional logic to determine which bits in the byte are used
                // or additional syntax in the action bus assembly output.
                tbl->stage->action_bus_use_bit_mask.setrange(slot.byte * 8U, slot.size);
            }
            if (use[slotno]) {
                BUG_CHECK(!slot.data.empty() && !use[slotno]->data.empty());
                auto nsrc = slot.data.begin()->first;
                unsigned noff = slot.data.begin()->second;
                unsigned nstart = 8 * (byte - slot.byte) + noff;
                if (nsrc.type == ActionBusSource::Field) nstart = nsrc.field->immed_bit(nstart);
                auto osrc = use[slotno]->data.begin()->first;
                unsigned ooff = use[slotno]->data.begin()->second;
                unsigned ostart = 8 * (byte - use[slotno]->byte) + ooff;
                if (osrc.type == ActionBusSource::Field) {
                    if (ostart < osrc.field->size)
                        ostart = osrc.field->immed_bit(ostart);
                    else
                        ostart += osrc.field->immed_bit(0);
                }
                if (ostart != nstart)
                    error(lineno,
                          "Action bus byte %d used inconsistently for fields %s and "
                          "%s in table %s",
                          byte, use[slotno]->name.c_str(), slot.name.c_str(), tbl->name());
            } else {
                use[slotno] = &slot;
            }
            unsigned hi = slot.lo(tbl) + slot.size - 1;
            if (action_hv_slice_use.size() <= hi / 128U) action_hv_slice_use.resize(hi / 128U + 1);
            auto &hv_groups = action_hv_slice_byte_groups.at(slot.byte / 16);
            for (unsigned byte = slot.lo(tbl) / 8U; byte <= hi / 8U; ++byte) {
                byte_use[byte] = 1;
                action_hv_slice_use.at(byte / 16).at(slot.byte / 16) |= hv_groups.at(byte % 16);
            }
        }
    }
}

bool ActionBus::check_slot_sharing(Slot &slot, bitvec &action_bus) {
    return (action_bus.getrange(slot.byte * 8U, slot.size) == 0);
}

bool ActionBus::check_atcam_sharing(Table *tbl1, Table *tbl2) {
    bool atcam_share_bytes = false;
    bool atcam_action_share_bytes = false;
    // Check tables are not same atcam's sharing bytes on action bus
    if (tbl1->to<AlgTcamMatchTable>() && tbl2->to<AlgTcamMatchTable>() &&
        tbl1->p4_table->p4_name() == tbl2->p4_table->p4_name())
        atcam_share_bytes = true;
    // Check tables are not same atcam action tables sharing bytes on action bus
    if (auto tbl1_at = tbl1->to<ActionTable>()) {
        if (auto tbl2_at = tbl2->to<ActionTable>()) {
            auto tbl1_mt = tbl1_at->get_match_table();
            auto tbl2_mt = tbl2_at->get_match_table();
            if (tbl1_mt->p4_table->p4_name() == tbl2_mt->p4_table->p4_name())
                atcam_action_share_bytes = true;
        }
    }
    return (atcam_share_bytes || atcam_action_share_bytes);
}

void ActionBus::need_alloc(Table *tbl, const ActionBusSource &src, unsigned lo, unsigned hi,
                           unsigned size) {
    LOG3("need_alloc(" << tbl->name() << ") " << src << " lo=" << lo << " hi=" << hi << " size=0x"
                       << hex(size));
    need_place[src][lo] |= size;
    switch (src.type) {
        case ActionBusSource::Field:
            lo += src.field->immed_bit(0);
            break;
        case ActionBusSource::TableOutput:
            src.table->set_output_used();
            break;
        case ActionBusSource::TableColor:
            src.table->set_color_used();
            break;
        case ActionBusSource::TableAddress:
            src.table->set_address_used();
            break;
        case ActionBusSource::XcmpData:
            break;
        default:
            break;
    }
    byte_use.setrange(lo / 8U, size);
}

/**
 * find_free -- find a free slot on the action output bus for some data.  Looks through bytes
 * in the range min..max for a free space where we can put 'bytes' bytes from an action
 * input bus starting at 'lobyte'.  'step' is an optimization to only check every step bytes
 * as we know alignment restrictions mean those are the only possible aligned spots
 */
int ActionBus::find_free(Table *tbl, unsigned min, unsigned max, unsigned step, unsigned lobyte,
                         unsigned bytes) {
    unsigned avail;
    LOG4("find_free(" << min << ", " << max << ", " << step << ", " << lobyte << ", " << bytes
                      << ")");
    for (unsigned i = min; i + bytes - 1 <= max; i += step) {
        unsigned hv_slice = i / ACTION_HV_XBAR_SLICE_SIZE;
        auto &hv_groups = action_hv_slice_byte_groups.at(hv_slice);
        int mask1 = action_hv_slice_group_align.at(hv_slice).at(lobyte % 16U) - 1;
        int mask2 = action_hv_slice_group_align.at(hv_slice).at((lobyte + bytes - 1) % 16U) - 1;
        if ((i ^ lobyte) & mask1) continue;  // misaligned
        bool inuse = false;
        for (unsigned byte = lobyte & ~mask1; byte <= ((lobyte + bytes - 1) | mask2); ++byte) {
            if (!byte_use[byte]) continue;
            if (action_hv_slice_use.size() <= byte / 16U)
                action_hv_slice_use.resize(byte / 16U + 1);
            if (action_hv_slice_use.at(byte / 16U).at(hv_slice) & hv_groups.at(byte % 16U)) {
                LOG5("  input byte " << byte << " in use for hv_slice " << hv_slice);
                inuse = true;
                break;
            }
        }
        if (inuse) {
            // skip up to next hv_slice
            while ((i + step) / ACTION_HV_XBAR_SLICE_SIZE == hv_slice) i += step;
            continue;
        }
        for (unsigned byte = i & ~mask1; byte <= ((i + bytes - 1) | mask2); ++byte)
            if (tbl->stage->action_bus_use[Stage::action_bus_slot_map[byte]]) {
                LOG5("  output byte "
                     << byte << " in use by "
                     << tbl->stage->action_bus_use[Stage::action_bus_slot_map[byte]]->name());
                inuse = true;
                break;
            }
        if (inuse) continue;
        for (avail = 1; avail < bytes; avail++)
            if (tbl->stage->action_bus_use[Stage::action_bus_slot_map[i + avail]]) break;
        if (avail >= bytes) return i;
    }
    return -1;
}

/**
 * find_merge -- find any adjacent/overlapping data on the action input bus that means the
 * data at 'offset' actually already on the action output bus
 *   offset     offset (in bits) on the action input bus of the data we're interested in
 *   bytes      how many bytes of data on the action input bus
 *   use        bitmask of the sizes of phv that need to access this on the action output bus
 */
int ActionBus::find_merge(Table *tbl, int offset, int bytes, int use) {
    LOG4("find_merge(" << offset << ", " << bytes << ", " << use << ")");
    bool is_action_data = dynamic_cast<ActionTable *>(tbl) != nullptr;
    for (auto &alloc : by_byte) {
        if (use & 1) {
            if (alloc.first >= 32) break;
        } else if (use & 2) {
            if (alloc.first < 32) continue;
            if (alloc.first >= 96) break;
        }
        if (alloc.second.is_table_output()) continue;  // can't merge table output with immediate
        int inbyte = alloc.second.lo(tbl) / 8U;
        int align = 4;
        if (is_action_data)
            align = action_hv_slice_group_align.at(alloc.first / 16U).at(inbyte % 16U);
        int outbyte = alloc.first & ~(align - 1);
        inbyte &= ~(align - 1);
        if (offset >= inbyte * 8 && offset + bytes * 8 <= (inbyte + align) * 8)
            return outbyte + offset / 8 - inbyte;
    }
    return -1;
}

void ActionBus::do_alloc(Table *tbl, ActionBusSource src, unsigned use, int lobyte, int bytes,
                         unsigned offset) {
    LOG2("putting " << src << '(' << offset << ".." << (offset + bytes * 8 - 1) << ")["
                    << (lobyte * 8) << ".." << ((lobyte + bytes) * 8 - 1) << "] at action_bus "
                    << use);
    unsigned hv_slice = use / ACTION_HV_XBAR_SLICE_SIZE;
    auto &hv_groups = action_hv_slice_byte_groups.at(hv_slice);
    for (unsigned byte = lobyte; byte < unsigned(lobyte + bytes); ++byte) {
        if (action_hv_slice_use.size() <= byte / 16) action_hv_slice_use.resize(byte / 16 + 1);
        action_hv_slice_use.at(byte / 16).at(hv_slice) |= hv_groups.at(byte % 16);
    }
    while (bytes > 0) {
        int slot = Stage::action_bus_slot_map[use];
        int slotsize = Stage::action_bus_slot_size[slot];
        auto slot_tbl = tbl->stage->action_bus_use[slot];
        // Atcam tables are mutually exclusive and should be allowed to share
        // bytes on action bus
        if (slot_tbl && !Table::allow_bus_sharing(tbl, slot_tbl))
            BUG_CHECK(slot_tbl == tbl || slot_tbl->action_bus->by_byte.at(use).data.count(src));
        tbl->stage->action_bus_use[slot] = tbl;
        Slot &sl = by_byte.emplace(use, Slot(src.name(tbl), use, bytes * 8U)).first->second;
        if (sl.size < bytes * 8U) sl.size = bytes * 8U;
        sl.data.emplace(src, offset);
        LOG4("  slot " << sl.byte << "(" << sl.name << ") data += " << src.toString(tbl)
                       << " off=" << offset);
        offset += slotsize;
        bytes -= slotsize / 8U;
        use += slotsize / 8U;
    }
}

const unsigned ActionBus::size_masks[8] = {7, 7, 15, 15, 31, 31, 31, 31};

void ActionBus::alloc_field(Table *tbl, ActionBusSource src, unsigned offset,
                            unsigned sizes_needed) {
    LOG4("alloc_field(" << src << ", " << offset << ", " << sizes_needed << ")");
    int lineno = this->lineno;
    bool is_action_data = dynamic_cast<ActionTable *>(tbl) != nullptr;
    int lo, hi, use;
    bool can_merge = true;
    if (src.type == ActionBusSource::Field) {
        lo = src.field->immed_bit(offset);
        hi = src.field->immed_bit(src.field->size) - 1;
        lineno = tbl->find_field_lineno(src.field);
    } else {
        lo = offset;
        if (src.type == ActionBusSource::TableOutput || src.type == ActionBusSource::TableColor ||
            src.type == ActionBusSource::TableAddress || src.type == ActionBusSource::RandomGen)
            can_merge = false;
        if (src.type == ActionBusSource::HashDist &&
            !(src.hd->xbar_use & HashDistribution::IMMEDIATE_LOW))
            lo += 16;
        hi = lo | size_masks[sizes_needed];
    }
    if (lo / 32U != hi / 32U) {
        /* Can't go across 32-bit boundary so chop it down as needed */
        hi = lo | 31U;
    }
    int bytes = hi / 8U - lo / 8U + 1;
    int step = 4;
    if (is_action_data) step = (lo % 128U) < 32 ? 2 : (lo % 128U) < 64 ? 4 : 8;
    if (sizes_needed & 1) {
        /* need 8-bit */
        if ((lo % 8U) && (lo / 8U != hi / 8U)) {
            error(lineno,
                  "%s not correctly aligned for 8-bit use on "
                  "action bus",
                  src.toString(tbl).c_str());
            return;
        }
        unsigned start = (lo / 8U) % step;
        int bytes_needed = (sizes_needed & 4) ? bytes : 1;
        if ((use = find(tbl->stage, src, lo, hi, 1)) >= 0 ||
            (can_merge && (use = find_merge(tbl, lo, bytes_needed, 1)) >= 0) ||
            (use = find_free(tbl, start, 31, step, lo / 8U, bytes_needed)) >= 0)
            do_alloc(tbl, src, use, lo / 8U, bytes_needed, offset);
        else
            error(lineno, "Can't allocate space on 8-bit part of action bus for %s",
                  src.toString(tbl).c_str());
    }
    step = (lo % 128U) < 64 ? 4 : 8;
    if (sizes_needed & 2) {
        /* need 16-bit */
        if (lo % 16U) {
            if (lo / 16U != hi / 16U) {
                error(lineno,
                      "%s not correctly aligned for 16-bit use "
                      "on action bus",
                      src.toString(tbl).c_str());
                return;
            }
            if (can_merge && (use = find_merge(tbl, lo, bytes, 2)) >= 0) {
                do_alloc(tbl, src, use, lo / 8U, bytes, offset);
                return;
            }
        }
        if (!(sizes_needed & 4) && bytes > 2) bytes = 2;
        unsigned start = 32 + (lo / 8U) % step;
        if ((use = find(tbl->stage, src, lo, hi, 2)) >= 0 ||
            (can_merge && (use = find_merge(tbl, lo, bytes, 2)) >= 0) ||
            (use = find_free(tbl, start, 63, step, lo / 8U, bytes)) >= 0 ||
            (use = find_free(tbl, start + 32, 95, 8, lo / 8U, bytes)) >= 0)
            do_alloc(tbl, src, use, lo / 8U, bytes, offset);
        else
            error(lineno, "Can't allocate space on 16-bit part of action bus for %s",
                  src.toString(tbl).c_str());
    }
    if (sizes_needed == 4) {
        /* need only 32-bit */
        unsigned odd = (lo / 8U) & (4 & step);
        unsigned start = (lo / 8U) % step;
        if (lo % 32U) {
            if (can_merge && (use = find_merge(tbl, lo, bytes, 4)) >= 0) {
                do_alloc(tbl, src, use, lo / 8U, bytes, offset);
                return;
            }
        }
        if ((use = find(tbl->stage, src, lo, hi, 4)) >= 0 ||
            (can_merge && (use = find_merge(tbl, lo, bytes, 4)) >= 0) ||
            (use = find_free(tbl, 96 + start + odd, 127, 8, lo / 8U, bytes)) >= 0 ||
            (use = find_free(tbl, 64 + start + odd, 95, 8, lo / 8U, bytes)) >= 0 ||
            (use = find_free(tbl, 32 + start, 63, step, lo / 8U, bytes)) >= 0 ||
            (use = find_free(tbl, 0 + start, 31, step, lo / 8U, bytes)) >= 0)
            do_alloc(tbl, src, use, lo / 8U, bytes, offset);
        else
            error(lineno, "Can't allocate space on action bus for %s", src.toString(tbl).c_str());
    }
}

void ActionBus::pass3(Table *tbl) {
    bool is_action_data = dynamic_cast<ActionTable *>(tbl) != nullptr;
    LOG1("ActionBus::pass3(" << tbl->name() << ") " << (is_action_data ? "[action]" : "[immed]"));
    for (auto &d : need_place)
        for (auto &bits : d.second) alloc_field(tbl, d.first, bits.first, bits.second);
    int rnguse = -1;
    for (auto &slot : by_byte) {
        for (auto &d : slot.second.data) {
            if (d.first.type == ActionBusSource::RandomGen) {
                if (rnguse >= 0 && rnguse != d.first.rng.unit)
                    error(lineno, "Can't use both rng units in a single table");
                rnguse = d.first.rng.unit;
            }
        }
    }
}

static int slot_sizes[] = {
    5, /* 8-bit or 32-bit */
    6, /* 16-bit or 32-bit */
    6, /* 16-bit or 32-bit */
    4  /* 32-bit only */
};

/**
 * ActionBus::find
 * @brief find an action bus slot that contains the requested thing.
 *
 * Overloads allow looking for different kinds of things -- a Format::Field,
 * a HashDistribution, a RandomNumberGen, or something by name (generally a table output).
 * @param f     a Format::Field to look for
 * @param name  named slot to look for -- generally a table output, but may be a field
 * @param hd    a HashDistribution to look for
 * @param rng   a RandomNumberGen to look for
 * @param lo, hi range of bits in the thing specified by the first arg
 * @param size  bitmask of needed size classes -- 3 bits that denote need for a 8/16/32 bit
 *              actionbus slot.  Generally will only have 1 bit set, but might be 0.
 */
int ActionBus::find(const char *name, TableOutputModifier mod, int lo, int hi, int size, int *len) {
    if (auto *tbl = ::get(Table::all, name))
        return find(ActionBusSource(tbl, mod), lo, hi, size, -1, len);
    if (mod != TableOutputModifier::NONE) return -1;
    for (auto &slot : by_byte) {
        int offset = lo;
        if (slot.second.name != name) continue;
        if (size && !(size & static_cast<int>(slot_sizes[slot.first / 32U]))) continue;
        if (offset >= static_cast<int>(slot.second.size)) continue;
        if (len) *len = slot.second.size;
        return slot.first + offset / 8;
    }
    return -1;
}

int ActionBus::find(const ActionBusSource &src, int lo, int hi, int size, int pos, int *len) {
    bool hd1Found = true;
    int hd1Pos = -1;
    for (auto &slot : by_byte) {
        if (!slot.second.data.count(src)) continue;
        int offset = slot.second.data[src];
        // FIXME -- HashDist is 16 bits in either half of the 32-bit immediate path; we call
        // the high half (16..31), but we address it directly (as if it was 16 bits) for
        // non-32 bit accesses. So we ignore the top bit of the offset bit index when
        // accessing it for 8- or 16- bit slots.
        // There should be a better way of doing this.
        if ((src.type == ActionBusSource::HashDist || src.type == ActionBusSource::HashDistPair) &&
            size < 4)
            offset &= 15;
        // Table Color is 8 bits which is ORed into the top of the immediate;  The offset is
        // thus >= 24, but we want to ignore that here and just use the offset within the byte
        if (src.type == ActionBusSource::TableColor) offset &= 7;
        if (offset > lo) continue;
        if (offset + static_cast<int>(slot.second.size) <= hi) continue;
        if (size && !(size & slot_sizes[slot.first / 32U])) continue;
        if (len) *len = slot.second.size;
        auto bus_pos = slot.first + (lo - offset) / 8;
        if (pos >= 0 && bus_pos != pos) continue;
        return bus_pos;
    }
    return -1;
}

int ActionBus::find(Stage *stage, ActionBusSource src, int lo, int hi, int size, int *len) {
    int rv = -1;
    for (auto tbl : stage->tables)
        if (tbl->action_bus && (rv = tbl->action_bus->find(src, lo, hi, size, -1, len)) >= 0)
            return rv;
    return rv;
}

template <class REGS>
void ActionBus::write_action_regs(REGS &regs, Table *tbl, int home_row, unsigned action_slice) {
    LOG2("--- ActionBus write_action_regs(" << tbl->name() << ", " << home_row << ", "
                                            << action_slice << ")");
    bool is_action_data = dynamic_cast<ActionTable *>(tbl) != nullptr;
    auto &action_hv_xbar = regs.rams.array.row[home_row / 2].action_hv_xbar;
    unsigned side = home_row % 2; /* 0 == left,  1 == right */
    for (auto &el : by_byte) {
        if (!is_action_data && !el.second.is_table_output()) {
            // Nasty hack -- meter/stateful output uses the action bus on the meter row,
            // so we need this routine to set it up, but we only want to do it for the
            // meter bus output; the rest of this ActionBus is for immediate data (set
            // up by write_immed_regs below)
            continue;
        }
        LOG5("    " << el.first << ": " << el.second);
        unsigned byte = el.first;
        BUG_CHECK(byte == el.second.byte);
        unsigned slot = Stage::action_bus_slot_map[byte];
        unsigned bit = 0, size = 0;
        std::string srcname;
        for (auto &data : el.second.data) {
            // FIXME -- this loop feels like a hack -- the size SHOULD already be set in
            // el.second.size (the max of the sizes of everything in the data we're looping
            // over), so should not need recomputing.  We do need to figure out the source
            // bit location, and ignore things in other wide words, but that should be stored
            // in the Slot object?  What about wired-ors, writing two inputs to the same
            // slot -- it is possible but is it useful?
            unsigned data_bit = 0, data_size = 0;
            if (data.first.type == ActionBusSource::Field) {
                auto f = data.first.field;
                if ((f->bit(data.second) >> 7) != action_slice) continue;
                data_bit = f->bit(data.second) & 0x7f;
                data_size = std::min(el.second.size, f->size - data.second);
                srcname = "field " + tbl->find_field(f);
            } else if (data.first.type == ActionBusSource::TableOutput) {
                if (data.first.table->home_row() != home_row) {
                    // skip tables not on this home row
                    continue;
                }
                data_bit = data.second;
                data_size = el.second.size;
                srcname = "table " + data.first.table->name_;
            } else {
                // HashDist and RandomGen only work in write_immed_regs
                BUG();
            }
            LOG3("    byte " << byte << " (slot " << slot << "): " << srcname << " (" << data.second
                             << ".." << (data.second + data_size - 1) << ")" << " [" << data_bit
                             << ".." << (data_bit + data_size - 1) << "]");
            if (size) {
                BUG_CHECK(bit == data_bit);  // checked in pass1; maintained by pass3
                size = std::max(size, data_size);
            } else {
                bit = data_bit;
                size = data_size;
            }
        }
        if (size == 0) continue;
        if (bit + size > 128) {
            error(lineno,
                  "Action bus setup can't deal with field %s split across "
                  "SRAM rows",
                  el.second.name.c_str());
            continue;
        }
        unsigned bytemask = (1U << ((size + 7) / 8U)) - 1;
        switch (Stage::action_bus_slot_size[slot]) {
            case 8:
                for (unsigned sbyte = bit / 8; sbyte <= (bit + size - 1) / 8;
                     sbyte++, byte++, slot++) {
                    unsigned code = 0, mask = 0;
                    switch (sbyte >> 2) {
                        case 0:
                            code = sbyte >> 1;
                            mask = 1;
                            break;
                        case 1:
                            code = 2;
                            mask = 3;
                            break;
                        case 2:
                        case 3:
                            code = 3;
                            mask = 7;
                            break;
                        default:
                            BUG();
                    }
                    if ((sbyte ^ byte) & mask) {
                        error(lineno, "Can't put field %s into byte %d on action xbar",
                              el.second.name.c_str(), byte);
                        break;
                    }
                    auto &ctl = action_hv_xbar.action_hv_ixbar_ctl_byte[side];
                    switch (code) {
                        case 0:
                            ctl.action_hv_ixbar_ctl_byte_1to0_ctl = slot / 2;
                            ctl.action_hv_ixbar_ctl_byte_1to0_enable = 1;
                            break;
                        case 1:
                            ctl.action_hv_ixbar_ctl_byte_3to2_ctl = slot / 2;
                            ctl.action_hv_ixbar_ctl_byte_3to2_enable = 1;
                            break;
                        case 2:
                            ctl.action_hv_ixbar_ctl_byte_7to4_ctl = slot / 4;
                            ctl.action_hv_ixbar_ctl_byte_7to4_enable = 1;
                            break;
                        case 3:
                            ctl.action_hv_ixbar_ctl_byte_15to8_ctl = slot / 8;
                            ctl.action_hv_ixbar_ctl_byte_15to8_enable = 1;
                            break;
                    }
                    if (!(bytemask & 1))
                        LOG1("WARNING: " << SrcInfo(lineno) << ": putting " << el.second.name
                                         << " on action bus byte " << byte
                                         << " even though bit in bytemask is "
                                            "not set");
                    action_hv_xbar.action_hv_ixbar_input_bytemask[side] |= 1 << sbyte;
                    bytemask >>= 1;
                }
                break;
            case 16:
                byte &= ~1;
                slot -= ACTION_DATA_8B_SLOTS;
                bytemask <<= ((bit / 8) & 1);
                for (unsigned word = bit / 16; word <= (bit + size - 1) / 16;
                     word++, byte += 2, slot++) {
                    unsigned code = 0, mask = 0;
                    switch (word >> 1) {
                        case 0:
                            code = 1;
                            mask = 3;
                            break;
                        case 1:
                            code = 2;
                            mask = 3;
                            break;
                        case 2:
                        case 3:
                            code = 3;
                            mask = 7;
                            break;
                        default:
                            BUG();
                    }
                    if (((word << 1) ^ byte) & mask) {
                        error(lineno, "Can't put field %s into byte %d on action xbar",
                              el.second.name.c_str(), byte);
                        break;
                    }
                    auto &ctl = action_hv_xbar.action_hv_ixbar_ctl_halfword[slot / 8][side];
                    unsigned subslot = slot % 8U;
                    switch (code) {
                        case 1:
                            ctl.action_hv_ixbar_ctl_halfword_3to0_ctl = subslot / 2;
                            ctl.action_hv_ixbar_ctl_halfword_3to0_enable = 1;
                            break;
                        case 2:
                            ctl.action_hv_ixbar_ctl_halfword_7to4_ctl = subslot / 2;
                            ctl.action_hv_ixbar_ctl_halfword_7to4_enable = 1;
                            break;
                        case 3:
                            ctl.action_hv_ixbar_ctl_halfword_15to8_ctl = subslot / 4;
                            ctl.action_hv_ixbar_ctl_halfword_15to8_enable = 1;
                            break;
                    }
                    action_hv_xbar.action_hv_ixbar_input_bytemask[side] |= (bytemask & 3)
                                                                           << (word * 2);
                    bytemask >>= 2;
                }
                break;
            case 32: {
                byte &= ~3;
                slot -= ACTION_DATA_8B_SLOTS + ACTION_DATA_16B_SLOTS;
                unsigned word = bit / 32;
                unsigned code = 1 + word / 2;
                bit %= 32;
                bytemask <<= bit / 8;
                if (((word << 2) ^ byte) & 7) {
                    error(lineno, "Can't put field %s into byte %d on action xbar",
                          el.second.name.c_str(), byte);
                    break;
                }
                auto &ctl = action_hv_xbar.action_hv_ixbar_ctl_word[slot / 4][side];
                slot %= 4U;
                switch (code) {
                    case 1:
                        ctl.action_hv_ixbar_ctl_word_7to0_ctl = slot / 2;
                        ctl.action_hv_ixbar_ctl_word_7to0_enable = 1;
                        break;
                    case 2:
                        ctl.action_hv_ixbar_ctl_word_15to8_ctl = slot / 2;
                        ctl.action_hv_ixbar_ctl_word_15to8_enable = 1;
                        break;
                }
                action_hv_xbar.action_hv_ixbar_input_bytemask[side] |= (bytemask & 15)
                                                                       << (word * 4);
                bytemask >>= 4;
                break;
            }
            default:
                BUG();
        }
        if (bytemask)
            LOG1("WARNING: " << SrcInfo(lineno) << ": excess bits " << hex(bytemask)
                             << " set in bytemask for " << el.second.name);
    }
}
FOR_ALL_REGISTER_SETS(INSTANTIATE_TARGET_TEMPLATE, void ActionBus::write_action_regs, mau_regs &,
                      Table *, int, unsigned)

template <class REGS>
void ActionBus::write_immed_regs(REGS &regs, Table *tbl) {
    LOG2("--- ActionBus write_immed_regs(" << tbl->name() << ")");
    auto &adrdist = regs.rams.match.adrdist;
    int tid = tbl->logical_id;
    unsigned rngmask = 0;
    for (auto &f : by_byte) {
        if (f.second.is_table_output()) continue;
        LOG5("    " << f.first << ": " << f.second);
        int slot = Stage::action_bus_slot_map[f.first];
        unsigned off = 0;
        unsigned size = f.second.size;
        if (!f.second.data.empty()) {
            off = f.second.data.begin()->second;
            if (f.second.data.begin()->first.type == ActionBusSource::Field)
                off -= f.second.data.begin()->first.field->immed_bit(0);
            for (auto &d : f.second.data) {
                if (d.first.type == ActionBusSource::RandomGen) {
                    rngmask |= d.first.rng.unit << 4;
                    rngmask |= ((1 << (size / 8)) - 1) << d.second / 8;
                }
            }
        }
        switch (Stage::action_bus_slot_size[slot]) {
            case 8:
                for (unsigned b = off / 8; b <= (off + size - 1) / 8; b++) {
                    BUG_CHECK((b & 3) == (slot & 3));
                    adrdist.immediate_data_8b_enable[tid / 8] |= 1U << ((tid & 7) * 4 + b);
                    // we write these ctl regs twice if we use both bytes in a pair.  That will
                    // cause a WARNING in the log file if both uses are the same -- it should be
                    // impossible to get an ERROR for conflicting uses, as that should have caused
                    // an error in pass1 above, and never made it to this point.
                    setup_muxctl(adrdist.immediate_data_8b_ixbar_ctl[tid * 2 + b / 2], slot++ / 4);
                }
                break;
            case 16:
                slot -= ACTION_DATA_8B_SLOTS;
                for (unsigned w = off / 16; w <= (off + size - 1) / 16; w++) {
                    BUG_CHECK((w & 1) == (slot & 1));
                    setup_muxctl(adrdist.immediate_data_16b_ixbar_ctl[tid * 2 + w], slot++ / 2);
                }
                break;
            case 32:
                slot -= ACTION_DATA_8B_SLOTS + ACTION_DATA_16B_SLOTS;
                setup_muxctl(adrdist.immediate_data_32b_ixbar_ctl[tid], slot);
                break;
            default:
                BUG();
        }
    }
    if (rngmask) {
        regs.rams.match.adrdist.immediate_data_rng_enable = 1;
        regs.rams.match.adrdist.immediate_data_rng_logical_map_ctl[tbl->logical_id / 4]
            .set_subfield(rngmask, 5 * (tbl->logical_id % 4U), 5);
    }
}
FOR_ALL_REGISTER_SETS(INSTANTIATE_TARGET_TEMPLATE, void ActionBus::write_immed_regs, mau_regs &,
                      Table *)

std::string ActionBusSource::name(Table *tbl) const {
    switch (type) {
        case Field:
            return tbl->find_field(field);
        case TableOutput:
        case TableColor:
        case TableAddress:
            return table->name();
        case NameRef:
        case ColorRef:
        case AddressRef:
            return name_ref->name;
        default:
            return "";
    }
}

std::string ActionBusSource::toString(Table *tbl) const {
    std::stringstream tmp;
    switch (type) {
        case None:
            return "<none source>";
        case Field:
            return tbl->find_field(field);
        case HashDist:
            tmp << "hash_dist " << hd->id;
            return tmp.str();
        case RandomGen:
            tmp << "rng " << rng.unit;
            return tmp.str();
        case TableOutput:
            return table->name();
        case TableColor:
            return table->name_ + " color";
        case TableAddress:
            return table->name_ + " address";
        case Ealu:
            return "ealu";
        case XcmpData:
            tmp << "xcmp(" << xcmp_group << ":" << xcmp_byte << ")";
            return tmp.str();
        case NameRef:
        case ColorRef:
        case AddressRef:
            tmp << "name ";
            if (name_ref)
                tmp << name_ref->name;
            else
                tmp << "(meter)";
            if (type == ColorRef) tmp << " color";
            if (type == AddressRef) tmp << " address";
            return tmp.str();
        default:
            tmp << "<invalid source " << int(type) << ">";
            return tmp.str();
    }
}

std::ostream &operator<<(std::ostream &out, TableOutputModifier mod) {
    switch (mod) {
        case TableOutputModifier::Color:
            out << " color";
            break;
        case TableOutputModifier::Address:
            out << " address";
            break;
        default:
            break;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const ActionBus::Slot &sl) {
    out << sl.name << " byte=" << sl.byte << " size=" << sl.size;
    for (auto &d : sl.data) out << "\n\t" << d.first << ": " << d.second;
    return out;
}

std::ostream &operator<<(std::ostream &out, const ActionBus &a) {
    for (auto &slot : a.by_byte) out << slot.first << ": " << slot.second << std::endl;
    for (auto &np : a.need_place) {
        out << np.first << " {";
        const char *sep = " ";
        for (auto &el : np.second) {
            out << sep << el.first << ":" << el.second;
            sep = ", ";
        }
        out << (sep + 1) << "}" << std::endl;
    }
    out << "byte_use: " << a.byte_use << std::endl;
    for (auto &hvslice : a.action_hv_slice_use) {
        for (auto v : hvslice) out << "  " << hex(v, 4, '0');
        out << std::endl;
    }
    return out;
}

void dump(const ActionBus *a) { std::cout << *a; }
