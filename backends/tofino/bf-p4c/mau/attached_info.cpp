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

#include "bf-p4c/mau/attached_info.h"

#include "bf-p4c/common/slice.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "bf-p4c/mau/table_placement.h"
#include "bf-p4c/phv/phv_fields.h"

/** The purpose of the ValidateAttachedOfSingleTable pass is to determine whether or not the
 * tables using stateful tables are allowed within Tofino.  Essentially the constraints are
 * the following:
 *  - Multiple counters, meters, or registers can be found on a table if they use the
 *    same exact addressing scheme.
 *  - A table can only have a meter, a stateful alu, or a selector, as they use
 *    the same address in match central
 *  - Indirect addresses for twoport tables require a per flow enable bit as well
 */

/* check if two attachments of the same type use identical addressing schemes and are the same
 * kind of table */
bool ValidateAttachedOfSingleTable::compatible(const IR::MAU::BackendAttached *ba1,
                                               const IR::MAU::BackendAttached *ba2) {
    if (typeid(*ba1->attached) != typeid(*ba2->attached)) return false;
    if (ba1->attached->direct != ba2->attached->direct) return false;
    if (ba1->addr_location != ba2->addr_location) return false;
    if (ba1->pfe_location != ba2->pfe_location) return false;
    if (ba1->type_location != ba2->type_location) return false;
    if (ba1->attached->direct) return true;
    if (auto *a1 = ba1->attached->to<IR::MAU::Synth2Port>()) {
        auto *a2 = ba2->attached->to<IR::MAU::Synth2Port>();
        if (a1->width != a2->width) return false;
    }
    // for indirect need to check that address expressions and enables are identical in
    // each action.  We do this in SetupAttachedAddressing::ScanActions as we don't have
    // the necessary information here
    return true;
}

void ValidateAttachedOfSingleTable::free_address(const IR::MAU::AttachedMemory *am,
                                                 addr_type_t type) {
    IR::MAU::Table::IndirectAddress ia;
    auto ba = findContext<IR::MAU::BackendAttached>();
    BUG_CHECK(ba, "Attached backend not found, for %1% table.", tbl->name);
    LOG3("Free address for attached " << am->name << " with size " << am->size << " and location "
                                      << ba->addr_location << " on table " << tbl->name);
    if (users[type] != nullptr) {
        if (!compatible(users[type], ba))
            error(ErrorType::ERR_INVALID,
                  "overlap. Both %1% and %2% require the %3% address hardware, and cannot be on "
                  "the same table %4%.",
                  am, users[type]->attached->name, addr_type_name(type), tbl->externalName());
        return;
    }
    users[type] = ba;

    if (!am->direct) {
        if (am->size <= 0) {
            error(ErrorType::ERR_NOT_FOUND, "indirect attached table %1%. Does not have a size.",
                  am);
            return;
        }
    }

    BUG_CHECK(am->direct == (IR::MAU::AddrLocation::DIRECT == ba->addr_location),
              "%s: "
              "Instruction Selection did not correctly set up the addressing scheme for %s",
              am->srcInfo, am->name);

    ia.shifter_enabled = true;
    ia.address_bits = std::max(ceil_log2(am->size), 10);

    bool from_hash = (ba->addr_location == IR::MAU::AddrLocation::HASH) ? true : false;

    if (ba->pfe_location == IR::MAU::PfeLocation::OVERHEAD) {
        if (from_hash) {
            if (!tbl->has_match_data()) {
                error(ErrorType::ERR_INVALID,
                      "When an attached memory %1% is addressed by hash and requires "
                      "per action enabling, then the table %2% must have match data",
                      am, tbl->externalName());
                return;
            }
        }
        ia.per_flow_enable = true;
    }

    if (type == METER && ba->type_location == IR::MAU::TypeLocation::OVERHEAD) {
        if (from_hash) {
            if (!tbl->has_match_data()) {
                error(ErrorType::ERR_INVALID,
                      "When an attached memory %1% is addressed by hash and requires "
                      "multiple meter_type, then the table %2% must have match data",
                      am, tbl->externalName());
                return;
            }
        }
        ia.meter_type_bits = 3;
    }

    ind_addrs[type] = ia;
    LOG3("\tSetting indirect address info { addr_bits : "
         << ia.address_bits << ", meter_type_bits : " << ia.meter_type_bits
         << ", per_flow_enable : " << ia.per_flow_enable);
}

bool ValidateAttachedOfSingleTable::preorder(const IR::MAU::Counter *cnt) {
    free_address(cnt, STATS);
    return false;
}

bool ValidateAttachedOfSingleTable::preorder(const IR::MAU::Meter *mtr) {
    free_address(mtr, METER);
    return false;
}

bool ValidateAttachedOfSingleTable::preorder(const IR::MAU::StatefulAlu *salu) {
    if (getParent<IR::MAU::BackendAttached>()->use != IR::MAU::StatefulUse::NO_USE)
        free_address(salu, METER);
    return false;
}

bool ValidateAttachedOfSingleTable::preorder(const IR::MAU::Selector *as) {
    free_address(as, METER);
    return false;
}

/**
 * Action data addresses currently do not require a check for PFE, as it is always defaulted
 * on at this point
 */
bool ValidateAttachedOfSingleTable::preorder(const IR::MAU::ActionData *ad) {
    BUG_CHECK(!ad->direct, "Cannot have a direct action data table before table placement");
    if (ad->size <= 0) error(ErrorType::ERR_NOT_FOUND, "size count in %2% %1%", ad, ad->kind());
    int vpn_bits_needed = std::max(10, ceil_log2(ad->size)) + 1;

    IR::MAU::Table::IndirectAddress ia;
    ia.address_bits = vpn_bits_needed;
    ia.shifter_enabled = true;
    ind_addrs[ACTIONDATA] = ia;
    return false;
}

bool SplitAttachedInfo::BuildSplitMaps::preorder(const IR::MAU::Table *tbl) {
    for (auto at : tbl->attached) self.attached_to_table_map[at->attached->name].insert(tbl);
    return true;
}

bool SplitAttachedInfo::ValidateAttachedOfAllTables::preorder(const IR::MAU::Table *tbl) {
    LOG3("ValidateAttachedOfAllTables on table : " << tbl->name);
    ValidateAttachedOfSingleTable::TypeToAddressMap ia;
    ValidateAttachedOfSingleTable validate_attached(ia, tbl);
    tbl->attached.apply(validate_attached);

    for (auto ba : tbl->attached) {
        auto *at = ba->attached;
        if (!FormatType_t::track(at)) continue;

        ValidateAttachedOfSingleTable::addr_type_t addr_type;
        if (at->is<IR::MAU::MeterBus2Port>()) {
            addr_type = ValidateAttachedOfSingleTable::METER;
        } else if (at->is<IR::MAU::Counter>()) {
            addr_type = ValidateAttachedOfSingleTable::STATS;
        } else if (at->is<IR::MAU::ActionData>()) {
            addr_type = ValidateAttachedOfSingleTable::ACTIONDATA;
        } else if (at->is<IR::MAU::Selector>()) {
            addr_type = ValidateAttachedOfSingleTable::METER;
        } else {
            BUG("Unhandled attached table type %s", at);
        }

        auto &addr_info = self.address_info_per_table[tbl->name][at->name];
        addr_info.address_bits = ia[addr_type].address_bits;
        LOG4("\tSetting address info for attached " << at->name);
    }

    return true;
}

bool SplitAttachedInfo::EnableAndTypesOnActions::preorder(const IR::MAU::Action *act) {
    if (findContext<IR::MAU::StatefulAlu>()) return false;

    auto tbl = findContext<IR::MAU::Table>();

    for (auto ba : tbl->attached) {
        auto *at = ba->attached;
        if (!FormatType_t::track(at)) continue;

        auto uai = at->unique_id();
        auto &addr_info = self.address_info_per_table[tbl->name][at->name];

        auto pfe_p = act->per_flow_enables.find(uai);
        if (pfe_p != act->per_flow_enables.end()) {
            if (!act->hit_only()) addr_info.always_run_on_miss = false;
            if (!act->miss_only()) addr_info.always_run_on_hit = false;
        }

        auto type_p = act->meter_types.find(uai);
        if (type_p != act->meter_types.end()) {
            int meter_type = static_cast<int>(type_p->second);
            if (!act->hit_only()) addr_info.types_on_miss.setbit(meter_type);
            if (!act->miss_only()) addr_info.types_on_hit.setbit(meter_type);
            // self.types_per_attached[at->name].setbit(meter_type);
        }
    }
    return false;
}

int SplitAttachedInfo::addr_bits_to_phv_on_split(const IR::MAU::Table *tbl,
                                                 const IR::MAU::AttachedMemory *at) const {
    if (address_info_per_table.count(tbl->name) == 0) return 0;
    auto &addr_info = address_info_per_table.at(tbl->name).at(at->name);
    return addr_info.address_bits;
}

bool SplitAttachedInfo::enable_to_phv_on_split(const IR::MAU::Table *tbl,
                                               const IR::MAU::AttachedMemory *at) const {
    if (address_info_per_table.count(tbl->name) == 0) return false;
    auto &addr_info = address_info_per_table.at(tbl->name).at(at->name);
    return !(addr_info.always_run_on_hit && addr_info.always_run_on_miss);
}

int SplitAttachedInfo::type_bits_to_phv_on_split(const IR::MAU::Table *tbl,
                                                 const IR::MAU::AttachedMemory *at) const {
    if (address_info_per_table.count(tbl->name) == 0) return 0;
    auto &addr_info = address_info_per_table.at(tbl->name).at(at->name);
    return (addr_info.types_on_hit | addr_info.types_on_miss).popcount() > 1 ? 3 : 0;
}

/**
 * Create an instruction set a new PHV field from the address provided by the control plane.
 * If the address is an ActionArg or a Constant, then the PHV field must be set to pass the
 * information to the next part of the program.
 *
 * PhvInfo is passed as an argument to potentially create an instruction only when the
 * field is in PHV.  This is necessary for some of the ActionFormat analysis when the parameter
 * does or does not yet exist.
 */
const IR::MAU::Instruction *SplitAttachedInfo::pre_split_addr_instr(
    const IR::MAU::Action *act, const IR::MAU::Table *tbl, const IR::MAU::AttachedMemory *at) {
    int addr_bits = addr_bits_to_phv_on_split(tbl, at);
    // BUG_CHECK(addr_bits > 0, "invalid addr_bits in split_index");
    if (addr_bits == 0) return nullptr;

    if (auto *sc = act->stateful_call(at->name)) {
        const IR::Expression *index = sc->index;
        if (index->is<IR::MAU::StatefulCounter>()) {
            // if the index is coming from a stateful counter, use the counter in the
            // first SALU stage, not the match stage
            return nullptr;
        }
        const IR::Expression *dest = split_index(at, tbl);
        int index_width = index->type->width_bits();
        if (index_width > addr_bits)
            index = MakeSlice(index, 0, addr_bits - 1);
        else if (index_width > 0 && index_width < addr_bits)
            dest = MakeSlice(dest, 0, index_width - 1);
        return new IR::MAU::Instruction(act->srcInfo, "set"_cs, dest, index);
    }
    return nullptr;
}

/**
 * @sa comments above pre_split_addr_instr
 */
const IR::MAU::Instruction *SplitAttachedInfo::pre_split_enable_instr(
    const IR::MAU::Action *act, const IR::MAU::Table *tbl, const IR::MAU::AttachedMemory *at) {
    if (!enable_to_phv_on_split(tbl, at)) return nullptr;

    bool enabled = act->per_flow_enables.count(at->unique_id()) > 0;
    // FIXME -- It's not necessary to set the ena to 0 (metadata init will do that always?), but
    // doing so causes problems with imem allocation (it incorrectly tries to pack instructions
    // in the same imem word.)
    if (!enabled) return nullptr;
    return new IR::MAU::Instruction(act->srcInfo, "set"_cs, split_enable(at, tbl),
                                    new IR::Constant(IR::Type::Bits::get(1), 1));
}

/**
 * @sa comments above pre_split_addr_instr
 */
const IR::MAU::Instruction *SplitAttachedInfo::pre_split_type_instr(
    const IR::MAU::Action *act, const IR::MAU::Table *tbl, const IR::MAU::AttachedMemory *at) {
    int type_bits = type_bits_to_phv_on_split(tbl, at);
    // BUG_CHECK(type_bits > 0, "invalid type_bits in split_type");
    if (type_bits == 0) return nullptr;

    bool enabled = act->per_flow_enables.count(at->unique_id()) > 0;
    if (enabled) {
        BUG_CHECK(act->meter_types.count(at->unique_id()) > 0,
                  "An enable stateful op %1% "
                  "in action %2% has no meter type?",
                  at->name, act->name);
        int meter_type = static_cast<int>(act->meter_types.at(at->unique_id()));
        return new IR::MAU::Instruction(
            act->srcInfo, "set"_cs, split_type(at, tbl),
            new IR::Constant(IR::Type::Bits::get(type_bits), meter_type));
    }
    return nullptr;
}

const IR::Expression *SplitAttachedInfo::split_enable(const IR::MAU::AttachedMemory *at,
                                                      const IR::MAU::Table *tbl) {
    if (!enable_to_phv_on_split(tbl, at)) return nullptr;
    auto &tv = index_tempvars[at->name];
    if (!tv.enable) {
        cstring enable_name = at->name + "$ena"_cs;
        tv.enable = new IR::TempVar(IR::Type::Bits::get(1), 0, enable_name);
    }
    phv.addTempVar(tv.enable, tbl->gress);
    return tv.enable;
}

const IR::Expression *SplitAttachedInfo::split_index(const IR::MAU::AttachedMemory *at,
                                                     const IR::MAU::Table *tbl) {
    int addr_bits = addr_bits_to_phv_on_split(tbl, at);
    // BUG_CHECK(addr_bits > 0, "invalid addr_bits in split_index");
    if (addr_bits == 0) return nullptr;
    auto &tv = index_tempvars[at->name];
    if (!tv.index) {
        cstring addr_name = at->name + "$index"_cs;
        tv.index = new IR::TempVar(IR::Type::Bits::get(addr_bits), false, addr_name);
    }
    phv.addTempVar(tv.index, tbl->gress);
    return tv.index;
}

const IR::Expression *SplitAttachedInfo::split_type(const IR::MAU::AttachedMemory *at,
                                                    const IR::MAU::Table *tbl) {
    int type_bits = type_bits_to_phv_on_split(tbl, at);
    // BUG_CHECK(type_bits > 0, "invalid type_bits in split_type");
    if (type_bits == 0) return nullptr;
    auto &tv = index_tempvars[at->name];
    if (!tv.type) {
        cstring type_name = at->name + "$type"_cs;
        tv.type = new IR::TempVar(IR::Type::Bits::get(type_bits), false, type_name);
    }
    phv.addTempVar(tv.type, tbl->gress);
    return tv.type;
}

/**
 * When a match table is in a separate stage than (part of) it's stateful ALU, the
 * IR::MAU::Table object must be split into mulitple IR::MAU::Table objects as part of
 * table placement.  The table cannot generate a meter_adr if there's no SALU in the
 * same stage, (as this will not be read by any ALU), but instead potentially pass
 * the address/pfe/type through PHV to a later table.  Mixed modes are possible with
 * some stateful in the current stage and some in later stages, which means tha address
 * needs to be both on the meter bus AND in phv.
 *
 * @sa The address/pfe/type pass requirements are detailed in attached_info.h file, but
 * will be generated if necessary
 */
const IR::MAU::Action *SplitAttachedInfo::create_split_action(const IR::MAU::Action *act,
                                                              const IR::MAU::Table *tbl,
                                                              FormatType_t format_type) {
    LOG1("Creating split action on table : " << tbl->name << " for action : " << act->name
                                             << " and format type : " << format_type);
    BUG_CHECK(format_type.valid(), "invalid format_type in create_split_action");
    if (format_type.normal()) return act;
    auto att = format_type.tracking(tbl);
    BUG_CHECK(format_type.num_attached() == static_cast<int>(att.size()),
              "attached table mismatch in create_split_action");
    auto *rv = act->clone();
    HasAttachedMemory earlier_stage, this_stage, later_stage;
    for (unsigned i = 0U; i < att.size(); ++i) {
        if (format_type.attachedEarlierStage(i)) earlier_stage.add(att.at(i));
        if (format_type.attachedThisStage(i)) this_stage.add(att.at(i));
        if (format_type.attachedLaterStage(i)) {
            later_stage.add(att.at(i));
            // Will not be exiting from this point, still have to run one final table
            rv->exitAction = false;
        }
    }

    if (!format_type.matchThisStage()) {
        rv->hit_allowed = true;
        rv->default_allowed = true;
        rv->init_default = true;
        rv->hit_path_imp_only = true;
        rv->disallowed_reason = "hit_path_only"_cs;
    }

    // Find instructions that depend on SALU/meter outputs and deal with them
    // FIXME -- will not work for an instruction dependent on two different SALU/meter
    // outputs unless those SALU/meters are all in the same set of stages.  Not possible
    // at the moment, as such SALU/meters would need compatible addressing (which
    // requires the same number of entries in each stage.)  Could add the ability to put
    // such tables in completely disjoint stages?
    for (auto it = rv->action.begin(); it != rv->action.end();) {
        auto instr = *it;
        instr->apply(earlier_stage);
        instr->apply(this_stage);
        instr->apply(later_stage);
        if (this_stage.found()) {
            // depends on an output in this stage
            if (auto ops = earlier_stage.found()) {
                if (instr->name == "set") {
                    BUG_CHECK(instr->operands.size() == 2, "wrong number of operands to set");
                    *it = new IR::MAU::Instruction(instr->srcInfo, "or"_cs, instr->operands.at(0),
                                                   instr->operands.at(0), instr->operands.at(1));
                } else if (instr->name == "or" || instr->name == "xor" || instr->name == "add" ||
                           instr->name == "sub" || instr->name == "sadds" ||
                           instr->name == "saddu" || instr->name == "ssubs" ||
                           instr->name == "ssubu" || (instr->name == "orca" && ops == 4) ||
                           (instr->name == "orcb" && ops == 2) ||
                           (instr->name == "andca" && ops == 2) ||
                           (instr->name == "andcb" && ops == 4)) {
                    // these are all ok
                } else {
                    error("Can't split %s across stages (so can't split %s)", act, tbl);
                }
            }
            it++;
        } else if (later_stage.found() || earlier_stage.found()) {
            // depends on a SALU/meter not present in this stage
            it = rv->action.erase(it);
        } else if (format_type.matchThisStage()) {
            // not related to any stateful/meter
            it++;
        } else {
            it = rv->action.erase(it);
        }
    }

    // Rewrite stateful calls as appropriate
    for (auto it = rv->stateful_calls.begin(); it != rv->stateful_calls.end();) {
        auto instr = *it;
        instr->apply(this_stage);
        instr->apply(earlier_stage);
        if (this_stage.found()) {
            if (instr->index->is<IR::MAU::StatefulCounter>() && !earlier_stage.found()) {
                // first SALU stage using stateful counter, so use the counter in this
                // stage (don't rewrite to use $index set in earlier stage)
            } else if (!format_type.matchThisStage()) {
                bool att_mem_found_in_this_stage = false;
                // An action can have multiple attached tables each tied to a different instruction.
                // E.g.
                //  action map_port_indexB()
                //  {
                //      banked_port_statsB.count(pkt_count_index);  // Counter
                //      latch_timestampB.execute(pkt_count_index);  // Stateful
                //  }
                // Each instruction will be linked to one of the attached tables (memories)
                // Loop through to find the one used for this instruction.
                // Report a BUG if none found
                for (auto *at : this_stage) {
                    att_mem_found_in_this_stage = this_stage.found(at);
                    if (!att_mem_found_in_this_stage) continue;
                    if (auto *tv = split_index(at, tbl)) {
                        // FIXME -- should only create this modified cloned call once?
                        auto *call = (*it)->clone();
                        call->index = new IR::MAU::HashDist(new IR::MAU::HashGenExpression(tv));
                        *it = call;
                    }
                    break;
                }
                BUG_CHECK(att_mem_found_in_this_stage, "Inconsistent stateful calls in %s", act);
            }
            it++;
        } else if (format_type.track(instr->attached_callee)) {
            // split to a different stage
            it = rv->stateful_calls.erase(it);
        } else if (format_type.matchThisStage()) {
            // direct attached in same stage as match
            it++;
        } else {
            // direct attached with no match (so remove)
            it = rv->stateful_calls.erase(it);
        }
    }
    if (!format_type.matchThisStage() && rv->stateful_calls.empty()) return nullptr;

    for (auto it = rv->per_flow_enables.begin(); it != rv->per_flow_enables.end();) {
        unsigned i = std::find_if(att.begin(), att.end(),
                                  [it](const IR::MAU::AttachedMemory *at) {
                                      return *it == at->unique_id();
                                  }) -
                     att.begin();
        if (i < att.size() && !format_type.attachedThisStage(i))
            it = rv->per_flow_enables.erase(it);
        else
            it++;
    }

    for (auto it = rv->meter_types.begin(); it != rv->meter_types.end();) {
        unsigned i = std::find_if(att.begin(), att.end(),
                                  [it](const IR::MAU::AttachedMemory *at) {
                                      return it->first == at->unique_id();
                                  }) -
                     att.begin();
        if (i < att.size() && !format_type.attachedThisStage(i))
            it = rv->meter_types.erase(it);
        else
            it++;
    }

    if (!format_type.matchThisStage()) rv->args.clear();

    // If table match is on current stage and an attached stage is
    // present in a later stage this means we may have to add pre-split
    // instrucitons in addition to the attached table execution on current
    // action
    // E.g. for a stateful call we have to set a $index and $ena to be
    // used in the next stage checks
    if (format_type.matchThisStage()) {
        for (unsigned i = 0U; i < att.size(); ++i) {
            if (format_type.attachedLaterStage(i)) {
                if (auto instr1 = pre_split_addr_instr(act, tbl, att.at(i)))
                    rv->action.push_back(instr1);
                if (auto instr2 = pre_split_enable_instr(act, tbl, att.at(i)))
                    rv->action.push_back(instr2);
                if (auto instr3 = pre_split_type_instr(act, tbl, att.at(i)))
                    rv->action.push_back(instr3);
            }
        }
    } else {
        // If a table match was in a previous stage and we have attached split
        // across current and next stages, we need to split index and set it for
        // the next stage.
        for (unsigned i = 0U; i < att.size(); ++i) {
            if (format_type.attachedThisStage(i) && format_type.attachedLaterStage(i)) {
                auto *idx = split_index(att.at(i), tbl);
                BUG_CHECK(idx,
                          "Cannot split index for table %1% on"
                          " attached table %2% with format type %3%",
                          tbl->name, att.at(i)->name, format_type);
                auto *adj_idx = new IR::MAU::StatefulCounter(idx->type, att.at(i));
                auto *set = new IR::MAU::Instruction("set"_cs, idx, adj_idx);
                rv->action.push_back(set);
            }
        }
    }

    LOG3("Created action : " << rv);
    return rv;
}

const IR::MAU::Action *SplitAttachedInfo::get_split_action(const IR::MAU::Action *act,
                                                           const IR::MAU::Table *tbl,
                                                           FormatType_t format_type) {
    LOG2("Getting split action on table : " << tbl->name << " for action : " << act->name
                                            << " and format type : " << format_type);
    auto key = std::make_tuple(tbl->name, act->name, format_type);
    if (!cache.count(key)) cache[key] = create_split_action(act, tbl, format_type);
    // FIXME -- need to refresh the PhvInfo in case it has been cleared and recomputed
    // and has no references to the tempvars used in this action.  Should be a better
    // way of doing this.
    for (auto *ba : tbl->attached) {
        auto tv = index_tempvars.find(ba->attached->name);
        if (tv != index_tempvars.end()) {
            if (tv->second.enable) phv.addTempVar(tv->second.enable, tbl->gress);
            if (tv->second.index) phv.addTempVar(tv->second.index, tbl->gress);
            if (tv->second.type) phv.addTempVar(tv->second.type, tbl->gress);
        }
    }
    return cache.at(key);
}

bool ActionData::FormatType_t::track(const IR::MAU::AttachedMemory *at) { return !at->direct; }

std::vector<const IR::MAU::AttachedMemory *> ActionData::FormatType_t::tracking(
    const IR::MAU::Table *tbl) {
    std::vector<const IR::MAU::AttachedMemory *> rv;
    if (tbl) {
        for (auto *ba : tbl->attached) {
            if (track(ba->attached)) {
                rv.push_back(ba->attached);
            }
        }
    }
    return rv;
}

ActionData::FormatType_t ActionData::FormatType_t::default_for_table(const IR::MAU::Table *tbl) {
    ActionData::FormatType_t rv;
    int shift = 3;
    rv.value = THIS_STAGE;
    for (auto *ba : tbl->attached) {
        if (!track(ba->attached)) continue;
        rv.value |= THIS_STAGE << shift;
        shift += 3;
    }
    return rv;
}

void ActionData::FormatType_t::initialize(const IR::MAU::Table *tbl, int entries, bool prev_stages,
                                          const attached_entries_t &attached) {
    value = 0;
    if (entries > 0 || !tbl->match_table) value |= THIS_STAGE;
    if (prev_stages) value |= EARLIER_STAGE;
    // FIXME -- when to set LATER_STAGE?  Do we ever care?
    int shift = 3;
    for (auto *ba : tbl->attached) {
        auto *at = ba->attached;
        if (!track(at)) continue;
        BUG_CHECK(shift < 30, "too many attached tables for %s", tbl);
        if (!attached.at(ba->attached).first_stage) value |= EARLIER_STAGE << shift;
        if (attached.at(ba->attached).entries > 0) value |= THIS_STAGE << shift;
        if (attached.at(ba->attached).need_more) value |= LATER_STAGE << shift;
        shift += 3;
    }
    check_valid(tbl);
}

void ActionData::FormatType_t::check_valid(const IR::MAU::Table *tbl) const {
    BUG_CHECK((value & MASK), "invalid(0%o) FormatType", value);
    BUG_CHECK((value & MASK) != (EARLIER_STAGE | LATER_STAGE),
              "invalid(0%o) FormatType: discontinuous match", value);
    unsigned idx = 0;
    auto att = tracking(tbl);
    for (uint32_t attached = value >> 3; attached; attached >>= 3, idx++) {
        BUG_CHECK(!tbl || idx < att.size(), "too many attached tables(%d) for %s", idx, tbl);
        BUG_CHECK((attached & MASK), "invalid(0%o) FormatType: no entries for attached #%d", value,
                  idx);
        BUG_CHECK((attached & MASK) != (EARLIER_STAGE | LATER_STAGE),
                  "invalid(0%o) FormatType: discontinuous attached #%d", value, idx);
        if (!tbl || (TablePlacement::can_duplicate(att.at(idx)) && (attached & MASK) == THIS_STAGE))
            continue;
        BUG_CHECK(!(value & LATER_STAGE) || !(attached & (THIS_STAGE | EARLIER_STAGE)),
                  "invalid(0%o) FormatType: attached #%d after match", value, idx);
        BUG_CHECK(!(value & THIS_STAGE) || !(attached & EARLIER_STAGE),
                  "invalid(0%o) FormatType: attached #%d after match", value, idx);
    }
    BUG_CHECK(!tbl || idx == att.size(), "not enough attached tables(%d) for %s", idx, tbl);
}

namespace ActionData {
std::ostream &operator<<(std::ostream &out, ActionData::FormatType_t ft) {
    return out << "FormatType(0" << std::oct << ft.value << std::dec << ")";
}
}  // namespace ActionData

std::string ActionData::FormatType_t::toString() const {
    std::stringstream tmp;
    tmp << *this;
    return tmp.str();
}

// Test to see if first potential allocation has more of something than the second.
// THIS IS NOT A CONSISTENT (PARTIAL) ORDER -- not suitable for use as map/set key
bool operator>(const attached_entries_t &a, const attached_entries_t &b) {
    BUG_CHECK(a.size() == b.size(), "comparing incomparable attached_entries_t");
    for (auto &ae : a) {
        if (b.count(ae.first) == 0) continue;
        if (ae.second.entries > b.at(ae.first).entries) return true;
    }
    return false;
}

std::ostream &operator<<(std::ostream &out, const attached_entries_element_t &att_elem) {
    out << " { " << att_elem.entries << ", need more: " << (att_elem.need_more ? "Y" : "N")
        << ", first stage: " << (att_elem.first_stage ? "Y" : "N");
    out << " } ";
    return out;
}

std::ostream &operator<<(std::ostream &out, const attached_entries_t &att_entries) {
    out << "Attached entries : [ ";
    for (const auto &a : att_entries)
        out << " ( Memory: " << a.first->toString() << " -> " << a.second << " )";
    out << " ] ";
    return out;
}
