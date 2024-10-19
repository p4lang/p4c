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

#include "bf-p4c/phv/analysis/build_mutex.h"

Visitor::profile_t BuildMutex::init_apply(const IR::Node *root) {
    auto rv = Inspector::init_apply(root);
    mutually_inclusive.clear();
    fields_encountered.clear();
    return rv;
}

void BuildMutex::mark(const PHV::Field *f) {
    if (!f) return;
    if (IgnoreField(f)) {
        LOG5("Ignoring field    " << f);
        return;
    } else {
        LOG5("Considering field " << f << " ( " << (f->pov ? "pov " : "")
                                  << (f->metadata ? "metadata " : "")
                                  << (!f->pov && !f->metadata ? "header " : "") << ")");
    }
    int new_field = f->id;
    fields_encountered[new_field] = true;
    mutually_inclusive[new_field] |= fields_encountered;
}

bool BuildMutex::preorder(const IR::MAU::Action *act) {
    act->action.visit_children(*this);
    return false;
}

bool BuildMutex::preorder(const IR::Expression *e) {
    if (auto *f = phv.field(e)) {
        mark(f);
        return false;
    }
    return true;
}

void BuildMutex::flow_merge(Visitor &other_) {
    BuildMutex &other = dynamic_cast<BuildMutex &>(other_);
    fields_encountered |= other.fields_encountered;
    mutually_inclusive |= other.mutually_inclusive;
}

void BuildMutex::flow_copy(::ControlFlowVisitor &other_) {
    BuildMutex &other = dynamic_cast<BuildMutex &>(other_);
    fields_encountered = other.fields_encountered;
    mutually_inclusive = other.mutually_inclusive;
}

void BuildMutex::end_apply() {
    LOG4("mutually exclusive fields:");
    for (auto it1 = fields_encountered.begin(); it1 != fields_encountered.end(); ++it1) {
        const PHV::Field *f1 = phv.field(*it1);
        CHECK_NULL(f1);
        if (neverOverlay[*it1]) {
            if (f1->overlayable) {
                warning("Ignoring pa_no_overlay for padding field %1%", f1->name);
            } else {
                LOG5("Excluding field from overlay: " << phv.field(*it1));
                continue;
            }
        }
        for (auto it2 = it1; it2 != fields_encountered.end(); ++it2) {
            if (*it1 == *it2) continue;

            // Consider fields marked neverOverlay to always be mutually inclusive.
            const PHV::Field *f2 = phv.field(*it2);
            CHECK_NULL(f2);
            if (neverOverlay[*it2]) {
                if (f2->overlayable) {
                    warning("Ignoring pa_no_overlay for padding field %1%", f2->name);
                } else {
                    LOG5("Excluding field from overlay: " << phv.field(*it2));
                    continue;
                }
            }

            if (!pragma.can_overlay(f1, f2)) continue;

            if (mutually_inclusive(*it1, *it2)) continue;

            mutually_exclusive(*it1, *it2) = true;
            LOG4("(" << f1->name << ", " << f2->name << ")");
        }
    }
}
