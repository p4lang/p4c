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

#pragma once

#include "bf-p4c/phv/utils/utils.h"

namespace PHV {

/// Keeps track of equivalence classes for the purpose of slice list allocation (for a given set
/// of start constraints).
/// For each class it is sufficient to check the slice list can be allocated only to the first
/// container, if it cannot, checking the other containers is pointless.
class ContainerEquivalenceTracker {
    using GressAssignment = PHV::Allocation::GressAssignment;
    using ExtractSource = PHV::Allocation::ExtractSource;
    using ContainerAllocStatus = PHV::Allocation::ContainerAllocStatus;
    enum class Parity : int8_t { Zero = 0, One = 1, Unspecified = -1 };

    struct ContainerClass {
        bool is_empty;
        Parity parity;
        PHV::Kind kind;
        GressAssignment gress;
        GressAssignment parser_group_gress;
        GressAssignment deparser_group_gress;
        ExtractSource parser_extract_group_source;

        ContainerClass(const PHV::Allocation &alloc, PHV::Container c, bool track_parity) {
            auto cstat = alloc.getStatus(c);
            BUG_CHECK(cstat, "no allocation status for container %1%", c);
            is_empty = cstat->alloc_status == ContainerAllocStatus::EMPTY;
            parity = track_parity ? Parity(c.index() % 2) : Parity::Unspecified;
            kind = c.type().kind();
            gress = cstat->gress;
            parser_group_gress = cstat->parserGroupGress;
            deparser_group_gress = cstat->deparserGroupGress;
            parser_extract_group_source = cstat->parserExtractGroupSource;
        }

        bool operator==(const ContainerClass &o) const { return as_tuple() == o.as_tuple(); }

        bool operator<(const ContainerClass &o) const { return as_tuple() < o.as_tuple(); }

        std::tuple<bool, Parity, PHV::Kind, GressAssignment, GressAssignment, GressAssignment,
                   ExtractSource>
        as_tuple() const {
            return std::make_tuple(is_empty, parity, kind, gress, parser_group_gress,
                                   deparser_group_gress, parser_extract_group_source);
        }
    };

 public:
    explicit ContainerEquivalenceTracker(const PHV::Allocation &alloc);

    /// When called with a container c, this will return either:
    /// - std::nullopt if the container is first of its class and should be checked for allocation
    /// - or a PHV::Container of a container used in some previous call of this function on this
    ///   instance that is equivalent to c.
    std::optional<PHV::Container> find_equivalent_tried_container(PHV::Container c);

    /// Never consider this container to be equivalent to any other. Useful to mark containers
    /// appearing in exact container constraints
    void exclude(PHV::Container c) {
        if (c != PHV::Container()) excluded.insert(c);
    }

    /// Invalidate this container for the purpose of equivalence. That is, this container will not
    /// be considered to be equivalent to other containers any more (its class is removed). This
    /// has to be used if the constraints are too complex to evaluate and require recursive
    /// allocation of other containers.
    void invalidate(PHV::Container c);

    /// Set that the slice list contains wide arithmetic and therefore the containers can only be
    /// equivalent with different containers of the same parity.
    void set_is_wide_arith() { wideArith = true; }

    /// Set that the slice list contains low part of wide arithmetic. In this case, we can't
    /// use equivalence tracking efficiently due to quirks in the metrics which can differ for
    /// otherwise equivalent fields if used in wide arithmerics.
    void set_is_wide_arith_low() { wideArithLow = true; }

 private:
    std::optional<PHV::Container> find_single(PHV::Container);
    ContainerClass get_class(PHV::Container c) {
        return ContainerClass(alloc, c, wideArith || (restrictW0 && c.is(PHV::Size::b8)));
    }

    std::map<ContainerClass, PHV::Container> equivalenceClasses;
    std::set<PHV::Container> excluded;
    const PHV::Allocation &alloc;
    bool wideArith = false;
    bool wideArithLow = false;
    /// For Tofino 2 we need to keep track of parity of 8bit containers & treat W0 specially
    /// In Tofino 2, all extractions happen using 16b extracts. So a 16-bit parser extractor
    /// extracts over a pair of even and odd phv 8-bit containers to perform 8-bit extraction. If
    /// any of 8-bit containers in the pair are in CLEAR_ON_WRITE mode, then both containers will
    /// be cleared everytime an extraction happens. In order to avoid this corruption, if one
    /// container in the pair in in CLEAR_ON_WRITE mode, the other is not used in parser.
    /// Initialized in .cpp.
    /// Due to a hardware limitation on Tofino 2 where PHV container W0 can unexpectedly be
    /// written to at the end of parser with value 0x0000000, W0 can never be set to clear on write,
    /// as this would corrupt the value of the field in this container.
    bool restrictW0;
};

}  // namespace PHV
