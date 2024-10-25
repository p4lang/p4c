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

#include "jbay_phv_analysis.h"

#include "bf-p4c/device.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"

Visitor::profile_t JbayPhvAnalysis::init_apply(const IR::Node *root) {
    profile_t rv = Inspector::init_apply(root);
    fieldUsesMap.clear();
    phvMap.clear();
    listOfTables.clear();
    tableDepStage.clear();
    tableStack.clear();
    LOG4("-----------------------------");
    LOG4("Starting Tofino2 PHV Analysis");
    LOG4("-----------------------------");
    LOG4("\t" << totalBits() << "\tTotal bits in program");
    LOG4("\t" << totalAllocatedBits() << "\tTotal bits to be allocated");

    LOG4("");
    LOG4("\t" << totalPhvBits() << "\tPHV candidate bits");
    LOG4("\t" << totalTPhvBits() << "\tTPHV candidate bits");

    LOG4("");
    LOG4("\t" << totalHeaderBits() << "\tHeader bits");
    LOG4("\t" << totalMetadataBits() << "\tMetadata bits");
    LOG4("\t  " << totalPOVBits() << "\tPOV bits");

    LOG4("");
    LOG4("\t " << usedInPardeMetadata() << "\tMetadata used in PARDE");
    LOG4("\t" << usedInParde() << "\tBits used in PARDE");
    LOG4("\t" << usedInMau() << "\tBits used in MAU");
    LOG4("\t" << usedInPardeHeader() << "\tHeader Bits used in PARDE");
    LOG4("\t  " << usedInPardePOV() << "\tPOV Bits used in PARDE");

    LOG4("");
    LOG4("\t" << ingressPhvBits() << "\tIngress PHV Bits");
    LOG4("\t" << egressPhvBits() << "\tEgress PHV Bits");
    LOG4("\t" << ingressTPhvBits() << "\tIngress TPHV Bits");
    LOG4("\t " << egressTPhvBits() << "\tEgress TPHV Bits");

    LOG4("");
    LOG4("\t" << aluUseBits() << "\tBits used in ALU");
    LOG4("\t" << aluUseBitsHeader() << "\tHeader Bits used in ALU");
    LOG4("\t" << aluUseBitsMetadata() << "\tMetadata Bits used in ALU");
    LOG4("\t  " << aluUseBitsPOV() << "\tPOV Bits used in ALU");
    return rv;
}

bool JbayPhvAnalysis::preorder(const IR::MAU::Table *tbl) {
    listOfTables.insert(tbl);
    tableStack.push_back(tbl);
    return true;
}

bool JbayPhvAnalysis::preorder(const IR::MAU::TableKey *read) {
    auto tbl = findContext<IR::MAU::Table>();
    const PHV::Field *f = phv.field(read->expr);
    BUG_CHECK(tbl != nullptr, "No associated table found for PHV analysis - %1%", read->expr);
    if (!f) warning("\t\tField read not found: %1% in table: %2%", read->expr, tbl->name);
    fieldUsesMap[f] |= IPXBAR;
    tableMatches[tableStack.back()].insert(f);
    fieldUses[f][dg.min_stage(tbl)] |= MATCH;
    return false;
}

bool JbayPhvAnalysis::preorder(const IR::MAU::Action *act) {
    auto tbl = tableStack.back();
    ordered_set<const PHV::Field *> actionWrites = actions.actionWrites(act);
    ordered_set<const PHV::Field *> actionReads = actions.actionReads(act);
    tablePHVReads[tbl].insert(actionReads.begin(), actionReads.end());
    tablePHVWrites[tbl].insert(actionWrites.begin(), actionWrites.end());
    for (auto *f : actionWrites) fieldUses[f][dg.min_stage(tbl)] |= WRITE;
    for (auto *f : actionReads) fieldUses[f][dg.min_stage(tbl)] |= READ;
    return false;
}

void JbayPhvAnalysis::postorder(const IR::MAU::Table *tbl) {
    if (tbl != tableStack.back())
        warning("Popping a different table to the one encountered in postorder");
    tableStack.pop_back();
}

void JbayPhvAnalysis::end_apply() {
    LOG4("");
    LOG4("\t" << matchingBitsTotal() << "\tUsed for matching");
    LOG4("");
    LOG4("\t" << determineDarkCandidates() << "\tCandidate bits for DARK");
    LOG4("\t" << determineMochaCandidates() << "\tCandidate bits for MOCHA");

    auto dep = longestDependenceChain();
    LOG4("\n");
    printFieldLiveness(dep);
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        assessCandidacy(&f, dep);
    }

    LOG4("\nPrinting Initial Candidacy\n");
    printCandidacy(dep);
    printStagewiseStats(dep);

    LOG4("\nPrinting Fixed Allocation\n");
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (phvMap[&f] == TPHV) continue;
        fixPHVAllocation(&f, dep);
        fixMochaAllocation(&f, dep);
    }
    printCandidacy(dep);
    printStagewiseStats(dep);

    LOG4("\nPrinting Fixed Mocha Allocation\n");
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (phvMap[&f] == TPHV) continue;
        fixMochaAllocation(&f, dep);
    }
    printCandidacy(dep);
    printStagewiseStats(dep);

    LOG4("");
    LOG4("----------------------------");
    LOG4("Exiting Tofino2 PHV Analysis");
    LOG4("----------------------------");
}

size_t JbayPhvAnalysis::totalBits() const {
    size_t rv = 0;
    for (auto &f : phv) rv += f.size;
    return rv;
}

size_t JbayPhvAnalysis::totalAllocatedBits() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (uses.is_referenced(&f)) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::totalPhvBits() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (isPHV(&f)) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::totalTPhvBits() {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (!isPHV(&f)) {
            phvMap[&f] = TPHV;
            rv += f.size;
        }
    }
    return rv;
}

size_t JbayPhvAnalysis::totalHeaderBits() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (isHeader(&f)) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::totalMetadataBits() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (f.metadata) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::totalPOVBits() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (f.pov) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::usedInPardeMetadata() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (f.metadata && uses.is_used_parde(&f)) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::usedInParde() {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (uses.is_used_parde(&f)) {
            fieldUsesMap[&f] |= PARDE;
            rv += f.size;
        }
    }
    return rv;
}

size_t JbayPhvAnalysis::usedInMau() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (uses.is_used_mau(&f)) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::usedInPardeHeader() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (uses.is_used_parde(&f) && isHeader(&f)) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::usedInPardePOV() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (f.pov && uses.is_used_parde(&f)) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::ingressPhvBits() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (isPHV(&f) && isGress(&f, INGRESS)) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::egressPhvBits() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (isPHV(&f) && isGress(&f, EGRESS)) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::ingressTPhvBits() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (!isPHV(&f) && isGress(&f, INGRESS)) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::egressTPhvBits() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (!isPHV(&f) && isGress(&f, EGRESS)) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::aluUseBits() {
    size_t rv = 0;
    for (auto &f : phv) {
        bool isUsedInALU = uses.is_used_alu(&f);
        if (!isUsedInALU) continue;
        if (uses.is_referenced(&f)) {
            rv += f.size;
            fieldUsesMap[&f] |= ALU;
        } else {
            warning("\tUnreferenced field used in ALU: %1%", f.name);
        }
    }
    return rv;
}

size_t JbayPhvAnalysis::aluUseBitsHeader() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!isHeader(&f)) continue;
        bool isUsedInALU = uses.is_used_alu(&f);
        if (!isUsedInALU) continue;
        if (uses.is_referenced(&f))
            rv += f.size;
        else
            warning("\tUnreferenced field used in ALU: %1%", f.name);
    }
    return rv;
}

size_t JbayPhvAnalysis::aluUseBitsMetadata() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!f.metadata) continue;
        bool isUsedInALU = uses.is_used_alu(&f);
        if (!isUsedInALU) continue;
        if (uses.is_referenced(&f))
            rv += f.size;
        else
            warning("\tUnreferenced field used in ALU: %1%", f.name);
    }
    return rv;
}

size_t JbayPhvAnalysis::matchingBitsTotal() {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (fieldUsesMap[&f] & IPXBAR) rv += f.size;
    }
    return rv;
}

size_t JbayPhvAnalysis::aluUseBitsPOV() const {
    size_t rv = 0;
    for (auto &f : phv) {
        if (!f.pov) continue;
        bool isUsedInALU = uses.is_used_alu(&f);
        if (!isUsedInALU) continue;
        if (uses.is_referenced(&f))
            rv += f.size;
        else
            warning("\tUnreferenced field used in ALU: %1%", f.name);
    }
    return rv;
}

size_t JbayPhvAnalysis::determineDarkCandidates() {
    size_t numBits = 0;
    size_t numFields = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (!(fieldUsesMap[&f] & PARDE)) {
            if (!(fieldUsesMap[&f] & ALU)) {
                phvMap[&f] = DARK;
                ++numFields;
                numBits += f.size;
            }
        }
    }
    LOG4("\t" << numFields << "\tNumber of fields for DARK");
    return numBits;
}

size_t JbayPhvAnalysis::determineMochaCandidates() {
    size_t numBits = 0;
    size_t numFields = 0;
    for (auto &f : phv) {
        if (!uses.is_referenced(&f)) continue;
        if (phvMap[&f] & DARK) continue;
        if ((!(fieldUsesMap[&f] & ALU)) && (fieldUsesMap[&f] & IPXBAR)) {
            ++numFields;
            numBits += f.size;
        }
    }
    LOG4("\t" << numFields << "\tNumber of fields for MOCHA");
    return numBits;
}

void JbayPhvAnalysis::printFieldLiveness(int maxStages) {
    size_t numFields = 0;
    size_t fieldsSize = 0;
    std::stringstream ss;
    ss << "|";
    for (int i = 0; i <= maxStages; i++) {
        if (i > 9)
            ss << " " << i << "|";
        else
            ss << " " << i << " |";
    }
    ss << " Field";
    LOG4(ss.str());
    for (auto f : fieldUses) {
        ++numFields;
        fieldsSize += f.first->size;
        std::stringstream ssRow;
        ssRow << "|";
        for (int i = 0; i <= maxStages; i++) {
            unsigned use = fieldUses[f.first][i];
            if (use & MATCH)
                ssRow << "M";
            else
                ssRow << " ";
            if (use & READ)
                ssRow << "R";
            else
                ssRow << " ";
            if (use & WRITE)
                ssRow << "W|";
            else
                ssRow << " |";
        }
        ssRow << " " << f.first->name;
        LOG4(ssRow.str());
    }
    LOG4("Number of Fields: " << numFields);
    LOG4("Number of field bits: " << fieldsSize);
}

void JbayPhvAnalysis::assessCandidacy(const PHV::Field *f, int maxStages) {
    ordered_set<int> unusedStages;
    ordered_set<int> matchStages;
    ordered_set<int> aluStages;
    for (int i = 0; i <= maxStages; i++) {
        // use represents the field's use in the ith stage
        if (phvMap[f] == TPHV) {
            candidateTypes[f][i] = TPHV;
            continue;
        }
        unsigned use = fieldUses[f][i];
        LOG5("Use of field " << f->name << " in stage " << i << ": " << use);
        if (use == 0) {
            LOG5("Inserting in unused stages");
            unusedStages.insert(i);
        }
        if (use & MATCH) {
            LOG5("Inserting in match stages");
            matchStages.insert(i);
        }
        if ((use & READ) || (use & WRITE)) {
            LOG5("Inserting in read and write stages");
            aluStages.insert(i);
        }
    }
    LOG5("Num in unused: " << unusedStages.size());
    LOG5("Num in match : " << matchStages.size());
    LOG5("Num in alu   : " << aluStages.size());
    for (int stage : aluStages) {
        LOG5("\tSetting to NORMAL PHV");
        candidateTypes[f][stage] = NORMAL;
    }
    for (int stage : matchStages) {
        if (candidateTypes[f][stage] == 0) {
            LOG5("\tSetting to MOCHA");
            candidateTypes[f][stage] = MOCHA;
        }
    }
    for (int stage : unusedStages) {
        LOG5("stage: " << stage);
        bool isParsedDeparsed = uses.is_used_parde(f);
        // If parsed, last stage and first stage should be at least mocha
        if (stage == 0 || stage == maxStages) {
            if (isParsedDeparsed) {
                LOG5("\tSetting to MOCHA");
                candidateTypes[f][stage] = MOCHA;
            } else {
                LOG5("\tSetting to DARK | MOCHA");
                candidateTypes[f][stage] = DARK | MOCHA;
            }
        } else {
            // For unused fields, it is safe to move them either to dark or mocha
            LOG5("\tSetting to DARK OR MOCHA");
            candidateTypes[f][stage] = DARK | MOCHA;
        }
    }
}

void JbayPhvAnalysis::fixPHVAllocation(const PHV::Field *f, int maxStages) {
    int lastFound = -1;
    int found = -1;
    if (candidateTypes[f][0] & TPHV) return;
    for (int i = 0; i <= maxStages; i++) {
        if (candidateTypes[f][i] & NORMAL) {
            lastFound = found;
            found = i;
        } else {
            continue;
        }
        if (lastFound == -1) {
            if ((found > 0) && (fieldUses[f][found] & READ)) candidateTypes[f][found - 1] = NORMAL;
            continue;
        }
        // Only change allocation if the distance (in stages) between two requirements of normal
        // PHVs is greater than 2 (allows for copying into dark/mocha)
        if (found - lastFound <= 3) {
            if (lastFound + 1 < found) candidateTypes[f][lastFound + 1] = NORMAL;
            if (found - 1 > lastFound) candidateTypes[f][found - 1] = NORMAL;
        }
    }
}

void JbayPhvAnalysis::fixMochaAllocation(const PHV::Field *f, int maxStages) {
    int lastFound = -1;
    int found = -1;
    if (candidateTypes[f][0] & TPHV) return;
    for (int i = 0; i <= maxStages; i++) {
        if ((candidateTypes[f][i] & MOCHA) && !(candidateTypes[f][i] & DARK)) {
            lastFound = found;
            found = i;
        } else {
            continue;
        }
        if (lastFound == -1) {
            if ((found > 0) && (fieldUses[f][found] & READ)) {
                if (candidateTypes[f][found - 1] != NORMAL) {
                    candidateTypes[f][found - 1] = MOCHA;
                }
            }
            continue;
        }
        // Only change allocation if the distance (in stages) between two requirements of normal
        // PHVs is greater than 2 (allows for copying into dark/mocha)
        if (found - lastFound <= 3) {
            if ((lastFound + 1 < found) && (candidateTypes[f][lastFound + 1] != NORMAL))
                candidateTypes[f][lastFound + 1] = NORMAL;
            if ((found - 1 > lastFound) && (candidateTypes[f][found - 1] != NORMAL))
                candidateTypes[f][found - 1] = NORMAL;
        }
    }
}

void JbayPhvAnalysis::printCandidacy(int maxStages) {
    size_t numFields = 0;
    size_t fieldsSize = 0;
    std::stringstream ss;
    ss << "|";

    stageNormal.clear();
    stageMocha.clear();
    stageDark.clear();
    stageEither.clear();

    for (int i = 0; i <= maxStages; i++) {
        if (i > 9)
            ss << " " << i << "|";
        else
            ss << " " << i << " |";
    }
    ss << " Size | Field";
    LOG4(ss.str());
    for (auto f : candidateTypes) {
        if (phvMap[f.first] == TPHV) continue;
        ++numFields;
        fieldsSize += f.first->size;
        std::stringstream ssRow;
        ssRow << "|";
        for (int i = 0; i <= maxStages; i++) {
            unsigned alloc = candidateTypes[f.first][i];
            LOG5("Alloc for field " << f.first->name << " and stage " << i << " : " << alloc);
            if (alloc & NORMAL) {
                ssRow << " P |";
                stageNormal[i] += f.first->size;
            } else if ((alloc & MOCHA) && (alloc & DARK)) {
                ssRow << "DM |";
                stageEither[i] += f.first->size;
            } else if (alloc & MOCHA) {
                ssRow << " M |";
                stageMocha[i] += f.first->size;
            } else if (alloc & DARK) {
                ssRow << " D |";
                stageDark[i] += f.first->size;
            } else if (alloc & TPHV) {
                ssRow << " T |";
            } else {
                warning("Don't see an allocation for this field");
            }
        }
        ssRow << " " << (boost::format("%3d") % f.first->size) << "b |\t" << f.first->name;
        LOG4(ssRow.str());
    }
    LOG4("Number of Fields: " << numFields);
    LOG4("Number of field bits: " << fieldsSize);
}

void JbayPhvAnalysis::printStagewiseStats(int maxStages) {
    LOG4("Stage |  P  |  M  |  D  |  E  | Total");
    for (int i = 0; i <= maxStages; i++) {
        std::stringstream ss;
        size_t normal = stageNormal[i];
        size_t dark = stageDark[i];
        size_t mocha = stageMocha[i];
        size_t either = stageEither[i];
        size_t total = normal + dark + mocha + either;
        ss << (boost::format("%6d") % i) << "|" << (boost::format("%5d") % normal) << "|"
           << (boost::format("%5d") % mocha) << "|" << (boost::format("%5d") % dark) << "|"
           << (boost::format("%5d") % either) << "|" << (boost::format("%5d") % total);
        LOG4(ss.str());
    }
}

int JbayPhvAnalysis::longestDependenceChain() const {
    LOG4("");
    LOG4("-----------------------");
    LOG4("Start of Table Analysis");
    LOG4("-----------------------");
    int maxStage = -1;
    for (auto *tbl : listOfTables) {
        auto minStage = dg.min_stage(tbl);
        if (minStage > maxStage) maxStage = minStage;
    }
    return maxStage;
}

bool JbayPhvAnalysis::isPHV(const PHV::Field *f) const {
    if (uses.is_used_mau(f) || f->pov || f->deparsed_to_tm()) return true;
    return false;
}

bool JbayPhvAnalysis::isHeader(const PHV::Field *f) const {
    if (!f->pov && !f->metadata) return true;
    return false;
}

bool JbayPhvAnalysis::isGress(const PHV::Field *f, gress_t gress) const {
    if (f->gress == gress) return true;
    return false;
}
