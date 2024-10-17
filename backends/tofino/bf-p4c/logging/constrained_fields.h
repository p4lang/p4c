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

#ifndef EXTENSIONS_BF_P4C_LOGGING_CONSTRAINED_FIELDS_H_
#define EXTENSIONS_BF_P4C_LOGGING_CONSTRAINED_FIELDS_H_

#include <map>

#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"
#include "phv_schema.h"

/**
 *  \brief Class with handle for logging constraints
 */
class LoggableEntity {
 public:
    using Constraint = Logging::Phv_Schema_Logger::Constraint;

 protected:
    Constraint *logger = nullptr;

 public:
    Constraint *getLogger() {
        return logger;
    }

    bool hasLoggedConstraints() const {
        return logger->get_base_constraint().size() > 0;
    }
};

class ConstrainedField;

/**
 *  \brief Class representing FieldSlice with constraints, which has
 *  handle for constraint logging. Contains non-group constraints.
 */
class ConstrainedSlice : public LoggableEntity, public LiftCompare<ConstrainedSlice> {
 private:
    const ConstrainedField *parent;
    le_bitrange range;

    Constraints::AlignmentConstraint alignment;
    Constraints::ContainerSizeConstraint containerSize;

 public:
    ConstrainedSlice(const ConstrainedField &parent, le_bitrange range);

    const le_bitrange &getRange() const                           { return range; }
    const ConstrainedField &getParent() const                     { return *parent; }

    /// Constraints
    void setAlignment(const Constraints::AlignmentConstraint &alignment);
    const Constraints::AlignmentConstraint &getAlignment() const  { return alignment; }

    void setContainerSize(const Constraints::ContainerSizeConstraint &containerSize);
    const Constraints::ContainerSizeConstraint getContainerSize() const { return containerSize; }

    /// Comparators implementation for LiftCompare
    bool operator<(const ConstrainedSlice &other) const override;
    bool operator==(const ConstrainedSlice &other) const override;
};

/**
 *  \brief Class representing PHV::Field with constraints and handle for constraint
 *  logging. Contains non-group constraints and a list of slices.
 */
class ConstrainedField : public LoggableEntity {
 private:
    cstring name;
    std::vector<ConstrainedSlice> slices;

    Constraints::SolitaryConstraint solitary;
    Constraints::AlignmentConstraint alignment;
    Constraints::DigestConstraint digest;
    Constraints::ContainerSizeConstraint containerSize;

    bool deparsedBottomBits = false;
    bool noSplit = false;
    bool noOverlay = false;
    bool exactContainer = false;
    bool noHoles = false;
    bool sameContainerGroup = false;

 public:
    ConstrainedField() {}
    explicit ConstrainedField(const cstring &name);

    const cstring &getName() const                                        { return name; }

    void addSlice(const ConstrainedSlice &slice);
    std::vector<ConstrainedSlice> &getSlices()                            { return slices; }
    const std::vector<ConstrainedSlice> &getSlices() const                { return slices; }

    /// Constraints
    void setSolitary(const Constraints::SolitaryConstraint &solitary);
    const Constraints::SolitaryConstraint &getSolitary() const            { return solitary; }

    void setAlignment(const Constraints::AlignmentConstraint &alignment);
    const Constraints::AlignmentConstraint &getAlignment() const          { return alignment; }

    void setDigest(const Constraints::DigestConstraint &digest);
    const Constraints::DigestConstraint &getDigest() const                { return digest; }

    void setContainerSize(const Constraints::ContainerSizeConstraint &containerSize);
    const Constraints::ContainerSizeConstraint getContainerSize() const { return containerSize; }

    void setBottomBits(bool b);
    bool hasBottomBits() const                                       { return deparsedBottomBits; }

    void setNoSplit(bool b);
    bool hasNoSplit() const                                               { return noSplit; }

    void setNoOverlay(bool b);
    bool hasNoOverlay() const                                             { return noOverlay; }

    void setExactContainer(bool b);
    bool hasExactContainer() const                                       { return exactContainer; }

    void setNoHoles(bool b);
    bool hasNoHoles() const                                               { return noHoles; }

    void setSameContainerGroup(bool b);
    bool hasSameContainerGroup()                                     { return sameContainerGroup; }
};

typedef std::map<cstring, ConstrainedField> ConstrainedFieldMap;

/**
 *  \brief Initialize map of constrained fields by information present in PhvInfo
 *  and their slices based on slicing computed by SuperClusters.
 */
class ConstrainedFieldMapBuilder {
 public:
    static ConstrainedFieldMap buildMap(const PhvInfo &phv,
                                        const std::list<PHV::SuperCluster*> &groups);
};

#endif  /* EXTENSIONS_BF_P4C_LOGGING_CONSTRAINED_FIELDS_H_ */
