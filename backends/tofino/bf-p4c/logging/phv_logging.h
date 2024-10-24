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

#ifndef BF_P4C_LOGGING_PHV_LOGGING_H_
#define BF_P4C_LOGGING_PHV_LOGGING_H_

#include <algorithm>
#include <string>

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/device.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/logging/constrained_fields.h"
#include "bf-p4c/logging/group_constraint_extractor.h"
#include "bf-p4c/logging/logging.h"
#include "bf-p4c/mau/action_analysis.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/phv/constraints/constraints.h"
#include "bf-p4c/phv/make_clusters.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/phv_spec.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "bf-p4c/phv/utils/utils.h"
#include "ir/ir.h"
#include "phv_schema.h"

using Logging::Phv_Schema_Logger;
using namespace P4;

/** This class gathers information related to the MAU use of various fields for use during Phv
 * Logging. This pass must be run prior to Instruction Adjustment because action analysis does not
 * currently handle some IR structures (e.g. MultiOperands) introduced in that pass. This split
 * pass structure allows us to use existing infrastructure to gather MAU related information.
 */
struct CollectPhvLoggingInfo : public MauInspector {
    /// Information about PHV fields.
    const PhvInfo &phv;
    const PhvUse &uses;
    const ReductionOrInfo &red_info;

    /// Mapping of table names in the backend to the P4 name.
    ordered_map<cstring, cstring> tableNames;

    /// Action-related and table-related maps.
    /// Mapping from an action to the table which may execute it.
    ordered_map<const IR::MAU::Action *, const IR::MAU::Table *> actionsToTables;

    /// Mapping from a FieldSlice to all the actions in which it is written.
    ordered_map<const PHV::FieldSlice, ordered_set<const IR::MAU::Action *>> sliceWriteToActions;

    /// Mapping from a FieldSlice to all the actions in which it is read.
    ordered_map<const PHV::FieldSlice, ordered_set<const IR::MAU::Action *>> sliceReadToActions;

    /// Mapping from a FieldSlice to all the tables in which it is used in the input crossbar.
    ordered_map<const PHV::FieldSlice, ordered_set<cstring>> sliceXbarToTables;

    /// Map of fields with their slices, containing snapshot of non-group constraints
    /// applied to them before first allocation
    ConstrainedFieldMap fieldConstraints;

    /// Superclusters used to extract certain group constraints (MAU groups, equivalent alignment)
    const std::list<PHV::SuperCluster *> *superclusters = nullptr;

    /// Pointer to pragma object computed during PHV Analysis
    const PHV::Pragmas *pragmas = nullptr;

    /// Extracted no pack (different container) constraints
    ordered_map<cstring, ordered_map<cstring, bool>> noPackConstraints;

    /// Extracted MAU group constraints
    MauGroupExtractor *mauGroupConstraints = nullptr;

    EquivalentAlignExtractor *equivAlignConstraints = nullptr;

    void collectConstraints();

    /// Clear all local state.
    profile_t init_apply(const IR::Node *root) override;
    /// Gather the P4 names of all tables allocated.
    bool preorder(const IR::MAU::Table *tbl) override;
    /// Gather action writes and reads related information and populate relevant data structures.
    bool preorder(const IR::MAU::Action *act) override;
    /// Gather information related to input xbar reads.
    bool preorder(const IR::MAU::TableKey *read) override;

    CollectPhvLoggingInfo(const PhvInfo &p, const PhvUse &u, const ReductionOrInfo &ri)
        : phv(p), uses(u), red_info(ri) {}

 private:
    /// Get a set of FieldSlices matching the field uses for writes/reads in actions and
    /// input xbar uses based on the Field expression
    ordered_set<PHV::FieldSlice> getSlices(const IR::Expression *, const IR::MAU::Table *);
};

/** This class is meant to generate logging information about PHV allocation and usage and output it
 * in JSON format defined in logging/phv_schema.json.
 */
class PhvLogging : public MauInspector {
 public:
    using Phv_Schema_Logger = Logging::Phv_Schema_Logger;
    using Structure = Phv_Schema_Logger::Structure;
    using Container = Phv_Schema_Logger::Container;
    using FieldInfo = Phv_Schema_Logger::FieldInfo;
    using FieldGroupItem = Phv_Schema_Logger::FieldGroupItem;
    using Field = Phv_Schema_Logger::Field;
    using Slice = Phv_Schema_Logger::Slice;
    using FieldSlice = Phv_Schema_Logger::FieldSlice;
    using SourceLocation = Phv_Schema_Logger::SourceLocation;
    using BoolConstraint = Phv_Schema_Logger::BoolConstraint;
    using IntConstraint = Phv_Schema_Logger::IntConstraint;
    using ListConstraint = Phv_Schema_Logger::ListConstraint;
    using Constraint = Phv_Schema_Logger::Constraint;
    using ParserLocation = Phv_Schema_Logger::ParserLocation;
    using DeparserLocation = Phv_Schema_Logger::DeparserLocation;
    using MAULocation = Phv_Schema_Logger::MAULocation;
    using ContainerSlice = Phv_Schema_Logger::ContainerSlice;
    using Access = Phv_Schema_Logger::Access;
    using Resources = Phv_Schema_Logger::Resources;
    struct PardeInfo {
        std::string unit;
        std::string parserState;

        bool operator<(PardeInfo other) const {
            if (unit != other.unit) return unit < other.unit;
            return parserState < other.parserState;
        }

        bool operator==(PardeInfo other) const {
            return (unit == other.unit && parserState == other.parserState);
        }

        explicit PardeInfo(std::string u, std::string name = "") : unit(u), parserState(name) {}
    };

    /** This class gathers defuse information about the fields which can then be used
     * during PHV logging.
     * This pass has to be run prior LowerParser pass which needs to update the names
     * of merged states during MergeLoweredParserStates pass.
     */
    class CollectDefUseInfo : public Visitor {
        /// Defuse information for PHV fields.
        const FieldDefUse &defuse;

        const IR::Node *apply_visitor(const IR::Node *n, const char *name = 0) override;

     public:
        void replace_parser_state_name(cstring old_name, cstring new_name);
        /** Maps field id to set of parser states names which define the field.
         *  Populated by values from located_defs in FieldDefUse.
         */
        ordered_map<int, ordered_set<cstring>> parser_defs;
        /** Maps field id to set of deparser names which the field is used in.
         *  Populated by values from located_uses in FieldDefUse.
         */
        ordered_map<int, ordered_set<cstring>> deparser_uses;

        explicit CollectDefUseInfo(const FieldDefUse &du) : defuse(du) {}
    };

    enum class ConstraintReason : std::size_t {
        SolitaryAlu = 0,
        SolitaryIntrinsic,
        SolitaryChecksum,
        SolitaryLastByte,
        SolitaryMirror,
        SolitaryContainerSize,
        SolitaryPragma,
        SolitaryExceptSameDigest,
        DifferentContainer,
        MAUGroup,
        NoSplit,
        ContainerSize,
        Alignment,
        NoOverlay,
        ExactContainer,
        EquivalentAlignment,
        NoHoles,
        SameContainerGroup
    };

 private:
    /// Information about PHV fields.
    const PhvInfo &phv;
    /// Information about CLOT-allocated fields.
    const ClotInfo &clot;
    /// Information collected about PHV fields usage.
    const CollectPhvLoggingInfo &info;
    /// Collected defuse information for PHV fields.
    const CollectDefUseInfo &defuseInfo;
    /// Table allocation.
    const ordered_map<cstring, ordered_set<int>> &tableAlloc;
    /// Table summary.
    const TableSummary &tableSummary;

    /// Unallocated slices.
    ordered_set<PHV::FieldSlice> unallocatedSlices;

    /// Logger class automatically generated from phv_schema.json.
    Phv_Schema_Logger logger;

    profile_t init_apply(const IR::Node *root) override {
        unallocatedSlices.clear();
        return Inspector::init_apply(root);
    }

    void end_apply(const IR::Node *root) override;

    /// Populates data structures related to PHV groups.
    void populateContainerGroups(cstring groupType);

    /// Container related methods.

    /// @returns a string indicating the type of the container.
    const char *containerType(const PHV::Container c) const;

    /// Field Slice related methods.
    /// @returns a Records::field_class_t value indicating the type of field slice.
    const char *getFieldType(const PHV::Field *f) const;

    /// @returns the gress for a field slice.
    const char *getGress(const PHV::Field *f) const;

    const char *getDeparserAccessType(const PHV::Field *f) const;

    // @returns the name with the prefix '.' stripped out
    std::string stripDotPrefix(const cstring name) const;

    void getAllParserDefs(const PHV::Field *f, ordered_set<PardeInfo> &rv) const;
    void getAllDeparserUses(const PHV::Field *f, ordered_set<PardeInfo> &rv) const;

    void addPardeReadsAndWrites(const PHV::Field *f, ordered_set<PardeInfo> &rv,
                                ContainerSlice *cs) const;
    void addTableKeys(const PHV::FieldSlice &sl, ContainerSlice *cs) const;
    void addVLIWReads(const PHV::FieldSlice &sl, ContainerSlice *cs) const;
    void addVLIWWrites(const PHV::FieldSlice &sl, ContainerSlice *cs) const;
    void addMutexFields(const PHV::AllocSlice &sl, ContainerSlice *cs) const;

    PHV::Field::AllocState getAllocatedState(const PHV::Field *f);

    /** An ordered map of field names and the fields that can be used
     * by various logging functions for specific purposes, headerFiels,
     * digestFields, etc.
     */
    ordered_map<cstring, ordered_set<const PHV::Field *>> getFields();

    /** An ordered map of field aliases and their fields
     */
    ordered_map<const PHV::Field *, const PHV::Field *> getFieldAliases();

    /**
     *  Transform @p f to loggable field info
     */
    FieldInfo *getFieldInfo(const PHV::Field *f) const;

    /**
     *  If @p f has srcInfo, return SourceLocation with fileName and sourceLine
     *  Otherwise return DummyFile:-1
     */
    SourceLocation *getSourceLoc(const PHV::Field *f) const;

    /** If @p f is a field then return a logger FieldSlice object to log the field
     * info. Substitue the field name with the one in its @a aliasSource field if
     * @p use_alias is set to true
     */
    FieldSlice *logFieldSlice(const PHV::Field *f, bool use_alias);

    /** If @p sl is a slice of the field's alloc slices then return a logger
     * FieldSlice object to log the field slice info. Substitue the field name
     * with the one in its @a aliasSource field if @p use_alias is set to true.
     */
    FieldSlice *logFieldSlice(const PHV::AllocSlice &sl, bool use_alias);

    /** If @p sl is a slice of the field's alloc slices then return a logger
     * ContainerSlice object to log the container slice info. Substitue the
     * field name with the one in its @a aliasSource field if @p use_alias is set
     * to true.
     */
    ContainerSlice *logContainerSlice(const PHV::AllocSlice &sl, bool use_alias);

    /// Add header-specific information to the logger object.
    void logHeaders();

    /// Add field-specific information to the logger object.
    void logFields();

    /// Add container-specific information to the logger object.
    void logContainers();

    // Add database of constraint reasons
    void logConstraintReasons();

    // Add field constraint information to the logger object
    void logFieldConstraints(const cstring &fieldName, Field *logger);

    void logSolitaryConstraints(ConstrainedField &field, const SourceLocation *srcLoc);

    void logNoPackConstraint(ConstrainedField &field, const FieldInfo *fieldInfo,
                             const SourceLocation *srcLoc);

    void logGroupConstraint(ConstrainedField &field, ListConstraint *c,
                            std::vector<ConstrainedSlice> &group);

    void logMauGroupConstraint(ConstrainedField &field, const SourceLocation *srcLoc);

    void logNoSplitConstraint(ConstrainedField &field, const SourceLocation *srcLoc);

    void logContainerSizeConstraint(ConstrainedField &field, const SourceLocation *srcLoc);

    void logAlignmentConstraint(ConstrainedField &field, const SourceLocation *srcLoc);

    void logNoOverlayConstraint(ConstrainedField &field, const SourceLocation *srcLoc);

    void logExactContainerConstraint(ConstrainedField &field, const SourceLocation *srcLoc);

    void logEquivalentAlignConstraint(ConstrainedField &field, const SourceLocation *srcLoc);

    void logNoHolesConstraint(ConstrainedField &field, const SourceLocation *srcLoc);

    void logSameContainerGroup(ConstrainedField &field, const SourceLocation *srcLoc);

    // Looks up item in db. If item is not there, it is added.
    // Index of the item is then returned.
    // This template is supposed to be used with FieldGroupItem
    // and FieldGroups
    template <typename T>
    int getDatabaseIndex(std::vector<T> &db, T item);

 public:
    explicit PhvLogging(const char *filename, const PhvInfo &p, const ClotInfo &ci,
                        const CollectPhvLoggingInfo &c, const CollectDefUseInfo &cdu,
                        const ordered_map<cstring, ordered_set<int>> &t, const TableSummary &ts);
};

#endif /* BF_P4C_LOGGING_PHV_LOGGING_H_ */
