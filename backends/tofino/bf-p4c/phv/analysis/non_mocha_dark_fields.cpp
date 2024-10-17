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

#include "bf-p4c/phv/analysis/non_mocha_dark_fields.h"
#include "bf-p4c/logging/event_logger.h"
#include "bf-p4c/mau/action_analysis.h"

Visitor::profile_t NonMochaDarkFields::init_apply(const IR::Node* root) {
    profile_t rv = Inspector::init_apply(root);

    nonDark.clear();
    nonMocha.clear();

    return rv;
}

bool NonMochaDarkFields::preorder(const IR::MAU::Action* act) {
    auto* tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl, "Could not find table");
    ActionAnalysis aa(phv, false, false, tbl, red_info);
    ActionAnalysis::FieldActionsMap fieldActionsMap;
    aa.set_field_actions_map(&fieldActionsMap);
    act->apply(aa);

    LOG5("\tAnalyzing action " << act->name << " in table " << tbl->name);
    for (auto& faEntry : fieldActionsMap) {
        LOG5("\tInstruction: " << faEntry.first);
        auto fieldAction = faEntry.second;

        const PHV::Field* write = phv.field(fieldAction.write.expr);
        BUG_CHECK(write, "Action %1% does not have a write?", fieldAction.write.expr);
        FieldDefUse::locpair wr_pair(tbl, fieldAction.write.expr);
        if (defuse.hasNonDarkContext(wr_pair)) nonDark[write->id][tbl] = WRITE;

        if (fieldAction.name != "set") {
            LOG5("\t  Field written by nonset: " << write);
            nonDark[write->id][tbl] = WRITE;
            nonMocha[write->id][tbl] = WRITE;
        }

        for (auto& readSrc : fieldAction.reads) {
            if (readSrc.type == ActionAnalysis::ActionParam::ACTIONDATA ||
                readSrc.type == ActionAnalysis::ActionParam::CONSTANT) {
                LOG5("\t  Field written by action data/constant: " << write);
                nonDark[write->id][tbl] = WRITE;

                auto annot = tbl->match_table->getAnnotations();
                if (auto s = annot->getSingle("use_hash_action"_cs)) {
                    auto pragma_val = s->expr.at(0)->to<IR::Constant>()->asInt();
                    if (pragma_val == 1) {
                        LOG5("\t  Field written by action data/constant through hash_action: "
                             << write);
                        nonMocha[write->id][tbl] = WRITE;
                    }
                }
            }

            if (readSrc.speciality != ActionAnalysis::ActionParam::NO_SPECIAL) {
                LOG5("\t  Field written by speciality: " << write);
                BUG_CHECK((nonDark.count(write->id) && nonDark[write->id].count(tbl)),
                          "NonDark ref not found for table %1%", tbl->name);
                nonMocha[write->id][tbl] = WRITE;
            }
        }
    }

    return true;
}

void NonMochaDarkFields::end_apply() {
    for (const PHV::Field& f : phv) {
        for (const FieldDefUse::locpair use : defuse.getAllUses(f.id)) {
            const IR::BFN::Unit* use_unit = use.first;
            if (use_unit->is<IR::BFN::ParserState>() || use_unit->is<IR::BFN::Parser>() ||
                use_unit->to<IR::BFN::GhostParser>()) {
                // Nothing to mark for nonDark
                LOG_DEBUG4(TAB1 "  Ignoring use in parser");
                continue;
            } else if (use_unit->is<IR::BFN::Deparser>()) {
                // Ignore deparser use
                LOG_DEBUG4(TAB1 "  Ignoring Use in deparser.");
                continue;
            } else if (use_unit->is<IR::MAU::Table>()) {
                const auto* tbl = use_unit->to<IR::MAU::Table>();
                LOG_DEBUG4(TAB1 "  Used in table " << tbl->name);
                if (!defuse.hasNonDarkContext(use)) {
                    LOG_DEBUG4("\t\tCan use in a dark container.");
                } else {
                    nonDark[f.id][tbl] |= READ;
                }
            } else {
                BUG("Unknown unit encountered %1%", use_unit->toString());
            }
        }
    }
}

PHV::FieldUse NonMochaDarkFields::isNotMocha(const PHV::Field* field,
                                             const IR::MAU::Table* tbl) const {
    if (!tbl) {
        LOG5(" tbl = null,  field = " << field->name
                                      << "   in nonMocha: " << nonMocha.count(field->id));
        return (nonMocha.count(field->id) ? PHV::FieldUse(PHV::FieldUse::READWRITE)
                                          : PHV::FieldUse());
    } else {
        LOG5(" tbl = " << tbl->name << ",  field = " << field->name
                       << "   in nonMocha: " << nonMocha.count(field->id));
        return (nonMocha.count(field->id)
                    ? (nonMocha.at(field->id).count(tbl) ? nonMocha.at(field->id).at(tbl)
                                                         : PHV::FieldUse())
                    : PHV::FieldUse());
    }
}

PHV::FieldUse NonMochaDarkFields::isNotDark(const PHV::Field* field,
                                            const IR::MAU::Table* tbl) const {
    if (tbl && nonDark.count(field->id) && nonDark.at(field->id).count(tbl)) {
        return nonDark.at(field->id).at(tbl);
    }
    return PHV::FieldUse();
}

const PHV::FieldUse NonMochaDarkFields::READ = PHV::FieldUse(PHV::FieldUse::READ);
const PHV::FieldUse NonMochaDarkFields::WRITE = PHV::FieldUse(PHV::FieldUse::WRITE);
