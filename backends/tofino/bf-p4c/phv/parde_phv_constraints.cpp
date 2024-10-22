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

#include "bf-p4c/phv/parde_phv_constraints.h"

Visitor::profile_t PardePhvConstraints::init_apply(const IR::Node *root) {
    profile_t rv = Inspector::init_apply(root);
    return rv;
}

bool PardePhvConstraints::preorder(const IR::BFN::Digest *digest) {
    // Currently only support these three digests.
    if (digest->name != "learning" && digest->name != "resubmit" && digest->name != "mirror")
        return true;
    PHV::Field *selector = phv.field(digest->selector->field);
    BUG_CHECK(selector, "Selector field not present in PhvInfo");
    // Selector field (rounded up to the nearest byte) is considered as part of the learning quanta
    // size. Note that this field is already assigned to an 8-bit container elsewhere in the
    // compiler.
    const unsigned selectorSize = 8 * ROUNDUP(static_cast<unsigned>(selector->size), 8);
    ;

    for (auto fieldList : digest->fieldLists) {
        unsigned digestSize = selectorSize;
        unsigned nonExactContainerDigestSize = 0;
        // Fields used in the digest.
        ordered_set<PHV::Field *> digestFields;
        // Fields used in the digest with a non exact containers requirement. It is only these
        // fields for whom we need to set constraints to limit the size of their containers.
        ordered_set<PHV::Field *> nonExactDigestFields;
        for (auto flval : fieldList->sources) {
            le_bitrange bits = {};
            PHV::Field *f = phv.field(flval->field, &bits);
            BUG_CHECK(f, "Digest field not present in PhvInfo");
            // The digest fields and digestSize objects are only maintained for logging purposes.
            // The constraints are all added over fields that do not have the exact_containers()
            // requirement.
            digestFields.insert(f);
            unsigned roundedUpSize = 8 * ROUNDUP(static_cast<unsigned>(bits.size()), 8);
            digestSize += roundedUpSize;
            if (f->exact_containers()) continue;
            nonExactDigestFields.insert(f);
            nonExactContainerDigestSize += roundedUpSize;
        }
        const unsigned digestSizeInBytes = digestSize / 8;

        if (digestSizeInBytes > Device::maxDigestSizeInBytes())
            error(
                "Learning quanta requires %1% bytes, which is greater than the maximum "
                "permissible %2% bytes.",
                digestSizeInBytes, Device::maxDigestSizeInBytes());

        if (LOGGING(3)) {
            LOG3("\tDigest size: " << digestSizeInBytes << "B.");
            LOG3("\t  Non exact containers digest size: " << (nonExactContainerDigestSize / 8)
                                                          << "B.");
            LOG3("\t  Selector field: " << selector->name);
            for (PHV::Field *f : digestFields) {
                std::stringstream ss;
                ss << "\t\t";
                if (nonExactDigestFields.count(f)) ss << "* ";
                ss << f->name << " (" << f->size << "b)";
                LOG3(ss.str());
            }
        }

        // If the digest size is less than the defined threshold, we don't need to impose any
        // additional constraints.
        if (digestSizeInBytes < DIGEST_BYTES_THRESHOLD) continue;

        // Impose constraints for digest fields without the exact_containers requirement.
        for (PHV::Field *f : nonExactDigestFields) {
            // For fields whose sizes are multiples of bytes, we set exact_containers to true and
            // also set them to solitary. This ensures that the containers with these fields do not
            // have any additional bits not occupied by slices of these fields.
            if (f->size % 8 == 0) {
                f->set_exact_containers(true);
                f->set_solitary(PHV::SolitaryReason::DIGEST);
                LOG2("\tSetting " << f->name << " to exact containers.");
                continue;
            }
            // For non byte multiple fields of size less than 32b, there are additional constraints.
            if (f->size <= 32) {
                const unsigned roundedUpSizeInBytes = ROUNDUP(static_cast<unsigned>(f->size), 8);
                BUG_CHECK(roundedUpSizeInBytes <= 4,
                          "The rounded up size of field %1% in bytes "
                          "here cannot be greater than 4.",
                          f->name);
                if (roundedUpSizeInBytes == 1) {
                    if (!sizePragmas.check_and_add_constraint(f, {PHV::Size::b8})) {
                        error(
                            "To fit within learning quanta size constraints, the field %1% must"
                            " occupy an 8-bit container. However, there are conflicting "
                            "pa_container_size constraints on the field",
                            f->name);
                        continue;
                    }
                    // Setting the fields to no split ensures that they will be assigned to the same
                    // container.
                    f->set_no_split(true);
                    LOG2("\tSetting " << f->name << " to 8-bit container and no-split");
                } else if (roundedUpSizeInBytes == 2) {
                    if (!sizePragmas.check_and_add_constraint(f, {PHV::Size::b16})) {
                        error(
                            "To fit within learning quanta size constraints, the field %1% must"
                            " occupy a 16-bit container. However, there are conficting "
                            "pa_container_size constraints on the field",
                            f->name);
                        continue;
                    }
                    // Setting the fields to no split ensures that they will be assigned to the same
                    // container.
                    f->set_no_split(true);
                    LOG2("\tSetting " << f->name << " to 16-bit container and no-split");
                } else if (roundedUpSizeInBytes == 3) {
                    // We cannot set no-split for this case as a field between 17-24 bits in size
                    // must necessarily be split across multiple containers. We choose one 16-bit
                    // and one 8-bit container just to avoid using too many containers of one size.
                    if (!sizePragmas.check_and_add_constraint(f, {PHV::Size::b16, PHV::Size::b8})) {
                        error(
                            "To fit within learning quanta size constraints, the field %1% must"
                            " occupy a 16-bit and an 8-bit container. However, there are "
                            "conflicting pa_container_size constraints on the field",
                            f->name);
                        continue;
                    }
                    LOG2("\tSetting " << f->name << " to 16-bit and 8-bit containers");
                } else {
                    if (!sizePragmas.check_and_add_constraint(f, {PHV::Size::b32})) {
                        error(
                            "To fit within learning quanta size constraints, the field %1% must"
                            " occupy a 32-bit container. However, there are conflicting "
                            "pa_container_size constraints on the field",
                            f->name);
                        continue;
                    }
                    LOG2("\tSetting " << f->name << " to 32-bit container");
                    // Setting the fields to no split ensures that they will be assigned to the same
                    // container.
                    f->set_no_split(true);
                    LOG2("\tSetting " << f->name << " to be no-split.");
                }
                continue;
            }
            // TODO: We do not have the ability to impose the right constraints on non byte
            // aligned fields of size greater than 32b. Maybe, the answer is to replace that single
            // field with multiple slices of the field and impose the constraints on those slices?
            warning(
                "Cannot yet impose constraints on non byte aligned field, %1% greater than "
                "32b.",
                f->name);
        }
    }
    return true;
}

Visitor::profile_t TofinoParserConstantExtract::init_apply(const IR::Node *root) {
    phv.clearConstantExtractionState();
    stateToPOVMap.clear();
    return Inspector::init_apply(root);
}

bool TofinoParserConstantExtract::preorder(const IR::BFN::Extract *e) {
    // The hardware constraints on extracting constants is not applicable to JBay.
    if (Device::currentDevice() != Device::TOFINO) return true;
    if (e->dest->is<IR::BFN::MatchLVal>()) return true;
    if (!e->dest) BUG("No destination for parser extract?");
    PHV::Field *field = phv.field(e->dest->field);
    if (!field) BUG("No field corresponding to parser extract destination?");
    // If the field is not being set to a constant value, ignore the extract.
    if (!e->source->to<IR::BFN::ConstantRVal>()) return true;
    // Only consider POV fields. For non POV fields, the parser should account for the extractors
    // correctly.
    if (!field->pov) return true;
    auto state = findContext<IR::BFN::ParserState>();
    if (!state) BUG("No parser state corresponding to extract?");
    LOG3("\t  Field extracted using constant: " << field->name << ", state: " << state->name);
    stateToPOVMap[state].insert(field);
    phv.insertConstantExtractField(field);
    return true;
}

void TofinoParserConstantExtract::end_apply() {
    for (auto kv : stateToPOVMap) {
        if (kv.second.size() == 1) continue;
        for (auto *f1 : kv.second) {
            for (auto *f2 : kv.second) {
                if (f1 == f2) continue;
                phv.mergeConstantExtracts(f1, f2);
            }
        }
    }

    if (!LOGGING(2)) return;

    LOG2("\tPrinting sets of fields extracted in the same state");
    UnionFind<PHV::Field *> &extracts = phv.getSameSetConstantExtraction();
    int i = 0;
    for (auto &set : extracts) {
        if (set.size() < 2) continue;
        LOG2("\t  Set " << i++);
        for (auto *f : set) LOG2("\t\t" << f->name);
    }
}
