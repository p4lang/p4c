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

#include "constrained_fields.h"

/* ConstrainedSlice */

ConstrainedSlice::ConstrainedSlice(const ConstrainedField &parent, le_bitrange range)
    : parent(&parent), range(range) {
    using Slice = Logging::Phv_Schema_Logger::Slice;
    using FieldSlice = Logging::Phv_Schema_Logger::FieldSlice;

    logger = new Constraint(new FieldSlice(std::string(stripThreadPrefix(parent.getName())),
                                           new Slice(range.lo, range.hi)));
}

void ConstrainedSlice::setAlignment(const Constraints::AlignmentConstraint &alignment) {
    ConstrainedSlice::alignment = alignment;
}

void ConstrainedSlice::setContainerSize(const Constraints::ContainerSizeConstraint &containerSize) {
    ConstrainedSlice::containerSize = containerSize;
}

bool ConstrainedSlice::operator<(const ConstrainedSlice &other) const {
    if (parent->getName() != other.parent->getName())
        return parent->getName() < other.parent->getName();
    return range < other.range;
}

bool ConstrainedSlice::operator==(const ConstrainedSlice &other) const {
    return parent->getName() == other.parent->getName() && range == other.range;
}

/* ConstrainedField */

ConstrainedField::ConstrainedField(const cstring &name) : name(name) {
    logger = new Constraint(nullptr);
}

void ConstrainedField::addSlice(const ConstrainedSlice &slice) { slices.push_back(slice); }

void ConstrainedField::setSolitary(const Constraints::SolitaryConstraint &solitary) {
    ConstrainedField::solitary = solitary;
}

void ConstrainedField::setAlignment(const Constraints::AlignmentConstraint &alignment) {
    ConstrainedField::alignment = alignment;
}

void ConstrainedField::setDigest(const Constraints::DigestConstraint &digest) {
    ConstrainedField::digest = digest;
}

void ConstrainedField::setContainerSize(const Constraints::ContainerSizeConstraint &containerSize) {
    ConstrainedField::containerSize = containerSize;
}

void ConstrainedField::setBottomBits(bool b) { deparsedBottomBits = b; }

void ConstrainedField::setNoSplit(bool b) { noSplit = b; }

void ConstrainedField::setNoOverlay(bool b) { noOverlay = b; }

void ConstrainedField::setExactContainer(bool b) { exactContainer = b; }

void ConstrainedField::setNoHoles(bool b) { noHoles = b; }

void ConstrainedField::setSameContainerGroup(bool b) { sameContainerGroup = b; }

/* ConstrainedFieldMap */

ConstrainedFieldMap ConstrainedFieldMapBuilder::buildMap(
    const PhvInfo &phv, const std::list<PHV::SuperCluster *> &groups) {
    ConstrainedFieldMap result;

    for (auto &f : phv) {
        result[f.name] = ConstrainedField(f.name);

        // Extract constraints for a field
        result[f.name].setSolitary(f.getSolitaryConstraint());

        if (f.alignment) {
            // NOTE: Bridge packing can overwrite alignment constraint of a field
            // so information about original reason can be lost
            Constraints::AlignmentConstraint alignment;
            alignment.addConstraint(f.getAlignmentConstraint().getReason(), f.alignment->align);
            result[f.name].setAlignment(alignment);
        }

        result[f.name].setDigest(f.getDigestConstraint());
        result[f.name].setBottomBits(f.deparsed_bottom_bits());
        result[f.name].setNoSplit(f.no_split());
        result[f.name].setExactContainer(f.exact_containers());
        result[f.name].setNoHoles(f.no_holes());
        result[f.name].setSameContainerGroup(f.same_container_group());
    }

    // Extract slices based on clustering
    for (auto sc : groups) {
        sc->forall_fieldslices([&](const PHV::FieldSlice &slice) {
            BUG_CHECK(result.find(slice.field()->name) != result.end(),
                      "Field is not present in PhvInfo, but is in supercluster.");
            auto &field = result.at(slice.field()->name);

            // Extract constraints for a slice
            ConstrainedSlice csl(field, slice.range());
            if (slice.alignment()) {
                Constraints::AlignmentConstraint alignment;
                alignment.addConstraint(field.getAlignment().getReason(), slice.alignment()->align);
                csl.setAlignment(alignment);
            }

            field.addSlice(csl);
        });
    }

    return result;
}
