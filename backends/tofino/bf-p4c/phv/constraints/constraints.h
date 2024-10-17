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

#ifndef BF_P4C_PHV_CONSTRAINTS_CONSTRAINTS_H_
#define BF_P4C_PHV_CONSTRAINTS_CONSTRAINTS_H_

#include <cstdint>
#include <ostream>
#include "lib/ordered_set.h"
#include "lib/cstring.h"

/// This is the file in which we will document all PHV constraints.
/// TODO: Integrate all constraints into this class format.

namespace PHV {

class Field;

}

namespace Constraints {

using namespace P4;

/// Class that captures whether a constraint is present on a field or not.
/// It also registers the reason for the constraint being added to the field.
/// reason = 0 necessarily implies the absence of the constraint.
class BooleanConstraint {
 protected:
    unsigned reason = 0;

 public:
    virtual bool hasConstraint() const = 0;
    virtual void addConstraint(uint32_t reason) = 0;
};

/// This class represents the solitary constraint, which implies that the field cannot be packed
/// with any other field in the same container. Note that solitary constraint does not preclude
/// fields sharing the same container through overlay.
class SolitaryConstraint : BooleanConstraint {
 public:
     // Define reasons for constraints here as enum classes.
    enum SolitaryReason {
        NONE = 0,                           // represents absence of solitary constraint
        ALU = (1 << 0),                     // solitary constraint due to ALU operation
        CHECKSUM = (1 << 1),                // solitary constraint due to use in checksum
        ARCH = (1 << 2),                    // solitary constraint required by the hardware
        DIGEST = (1 << 3),                  // solitary constraint due to use in digest
        PRAGMA_SOLITARY = (1 << 4),         // solitary constraint due to pa_solitary pragma
        PRAGMA_CONTAINER_SIZE = (1 << 5),   // solitary constraint due to pa_container_size
        CONFLICT_ALIGNMENT = (1 << 6),      // solitary constraint due to conflicting alignment
                                            // in bridge packing
        CLEAR_ON_WRITE = (1 << 7)           // solitary constraint due to the field being
                                            // cleared-on-write
    };

    bool hasConstraint() const { return (reason != 0); }
    void addConstraint(uint32_t r) { reason |= r; }

    bool isALU() const { return reason & ALU; }
    bool isChecksum() const { return reason & CHECKSUM; }
    bool isArch() const { return reason & ARCH; }
    bool isDigest() const { return reason & DIGEST; }
    bool isPragmaSolitary() const { return reason & PRAGMA_SOLITARY; }
    bool isPragmaContainerSize() const { return reason & PRAGMA_CONTAINER_SIZE; }
    bool isClearOnWrite() const { return reason & CLEAR_ON_WRITE; }
    bool isOnlyClearOnWrite() const { return reason == CLEAR_ON_WRITE; }
};

std::ostream &operator<<(std::ostream &out, const SolitaryConstraint& cons);

/// This class represents the digest constraint, which implies that the field is used in a digest.
/// Additionally, it also stores the type of digest in which the field is used.
class DigestConstraint : BooleanConstraint {
 public:
    // Define type of digest in which the field is used.
    enum DigestType {
        NONE = 0,               // Field is not used in a digest
        MIRROR = (1 << 0),      // used in mirror digest
        LEARNING = (1 << 1),    // used in learning digest
        RESUBMIT = (1 << 2),    // used in resubmit
        PKTGEN = (1 << 3)       // used in pktgen
    };

    bool hasConstraint() const { return (reason != 0); }
    void addConstraint(uint32_t r) { reason |= r; }

    bool isMirror() const { return reason & MIRROR; }
    bool isLearning() const { return reason & LEARNING; }
    bool isResubmit() const { return reason & RESUBMIT; }
    bool isPktGen() const { return reason & PKTGEN; }
};

class DeparsedToTMConstraint : BooleanConstraint {
 public:
    enum DeparsedToTMReason {
        NONE = 0,               // Field is not deparsed to TM
        DEPARSE = 1 << 0,       // Field is deparsed to TM
    };

    bool hasConstraint() const { return (reason != 0); }
    void addConstraint(uint32_t r) { reason |= r; }

    bool isDeparse() const { return reason & DEPARSE; }
};

class NoSplitConstraint : BooleanConstraint {
 public:
    enum NoSplitReason {
        NONE = 0,               // Field is not deparsed to TM
        NO_SPLIT = 1 << 0,      // Field is deparsed to TM
    };

    bool hasConstraint() const { return (reason != 0); }
    void addConstraint(uint32_t r) { reason |= r; }

    bool isNoSplit() const { return reason & NO_SPLIT; }
};

class IntegerConstraint {
 protected:
    unsigned reason = 0;
    unsigned value = 0;

 public:
    virtual ~IntegerConstraint() {}
    virtual bool hasConstraint() const = 0;
    virtual void addConstraint(unsigned r, unsigned v) = 0;
};

/// This class represents the alignment constraint, which implies that field must start at
/// a particular offset within a byte.
class AlignmentConstraint : IntegerConstraint {
 protected:
    // used by bridged packing to insert extra padding to ease phv allocation
    // see IMPL_NOTE(0) in bridged_packing.h
    // DO NOT OUTPUT THIS CONSTRAINT TO USER LOG
    unsigned container_size = 0;

 public:
    // Define the cause of alignment constraint
    enum AlignmentReason {
        NONE = 0,
        BRIDGE = (1 << 0),
        PARSER = (1 << 1),
        DEPARSER = (1 << 2),
        TERNARY_MATCH = (1 << 3),
        DIGEST = (1 << 4),
        INTRINSIC = (1 << 5),
        PA_BYTE_PACK = (1 << 6),
    };

    ~AlignmentConstraint() {}
    bool hasConstraint() const { return (reason != 0); }
    void addConstraint(unsigned source, unsigned v) { reason |= source; value = v; }

    void updateConstraint(unsigned source) { reason |= source; }
    void eraseConstraint() { reason = 0; }
    unsigned getAlignment() const { return value; }
    unsigned getReason() const { return reason; }

    void setContainerSize(unsigned size) { container_size = size; }
    unsigned getContainerSize() const { return container_size; }

    bool isBridged() const { return reason & BRIDGE; }
    bool isParser() const { return reason & PARSER; }
    bool isDeparser() const { return reason & DEPARSER; }
    bool isTernaryMatch() const { return reason & TERNARY_MATCH; }
    bool isDigest() const { return reason & DIGEST; }
    bool isIntrinsic() const { return reason & INTRINSIC; }

    bool operator==(const AlignmentConstraint & a) const {
        return reason == a.reason && value == a.value; }
    bool operator<(AlignmentConstraint const & a) const {
        if (value < a.value) return true;
        else if (reason < a.reason) return true;
        return false;
    }
};

class ContainerSizeConstraint : IntegerConstraint {
 public:
    enum ContainerSizeReason {
        NONE = 0,
        PRAGMA = 1
    };

    ~ContainerSizeConstraint() {}
    bool hasConstraint() const { return (reason != 0); }
    void addConstraint(unsigned source, unsigned v) { reason = source; value = v; }
    unsigned getContainerSize() const { return value; }
};

class GroupConstraint {
 protected:
    unsigned reason = 0;
    /* fields that share the same group constraint */
    ordered_set<const PHV::Field*> fields;

 public:
    virtual ~GroupConstraint() {}
    virtual bool hasConstraint() const = 0;
    virtual void addConstraint(uint32_t reason) = 0;
};

class PairConstraint {
 protected:
    unsigned reason = 0;
    // represents the pair of fields have the mutual constraint
    // e.g. A pair of field that must be packed to the same byte.
    //      A pair of field that must be be aligned to the same bit offset.
    std::pair<cstring, cstring> fields;

 public:
    PairConstraint(cstring f1, cstring f2) {
        if (f1 < f2) {
            fields.first = f1;
            fields.second = f2;
        } else {
            fields.first = f2;
            fields.second = f1;
        }
    }
    PairConstraint(PairConstraint& p) {
        fields.first = p.fields.first;
        fields.second = p.fields.second;
    }
    ~PairConstraint() {}
    virtual bool hasConstraint() const = 0;
    virtual void addConstraint(uint32_t reason) = 0;
    bool operator==(const PairConstraint & a) const {
        return reason == a.reason && fields == a.fields; }
    bool operator<(PairConstraint const & a) const {
        if (reason < a.reason) return true;
        else if (fields < a.fields) return true;
        return false;
    }
};

class CopackConstraint : PairConstraint {
 public:
    CopackConstraint(cstring f1, cstring f2) : PairConstraint(f1, f2) {}
    bool hasConstraint() const { return (reason != 0); }
    void addConstraint(unsigned r) { reason |= r; }
};

class NoOverlapConstraint : PairConstraint {
 public:
    NoOverlapConstraint(cstring f1, cstring f2) : PairConstraint(f1, f2) {}
    bool hasConstraint() const { return (reason != 0); }
    void addConstraint(unsigned r) { reason |= r; }
};

class NoPackConstraint : PairConstraint {
 public:
    NoPackConstraint(cstring f1, cstring f2) : PairConstraint(f1, f2) {}
    bool hasConstraint() const { return (reason != 0); }
    void addConstraint(unsigned r) { reason |= r; }
};

class MutuallyAlignedConstraint : PairConstraint {
 public:
    MutuallyAlignedConstraint(cstring f1, cstring f2) : PairConstraint(f1, f2) {}
    bool hasConstraint() const { return (reason != 0); }
    void addConstraint(unsigned r) { reason |= r; }
};


}  // namespace Constraints

#endif  /* BF_P4C_PHV_CONSTRAINTS_CONSTRAINTS_H_ */
