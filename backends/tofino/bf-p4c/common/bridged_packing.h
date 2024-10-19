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

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_BRIDGED_PACKING_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_BRIDGED_PACKING_H_

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/extract_maupipe.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"
#include "bf-p4c/logging/source_info_logging.h"
#include "bf-p4c/midend/blockmap.h"
#include "bf-p4c/midend/normalize_params.h"
#include "bf-p4c/midend/param_binding.h"
#include "bf-p4c/midend/simplify_references.h"
#include "bf-p4c/midend/type_checker.h"
#include "bf-p4c/phv/action_phv_constraints.h"
#include "bf-p4c/phv/constraints/constraints.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "z3++.h"

using namespace P4;

// Collection of bridge metadata constraints across all pipelines
// in the program.
struct AllConstraints {
    ordered_set<const Constraints::AlignmentConstraint *> alignedConstraints;
    ordered_set<const Constraints::CopackConstraint *> copackConstraints;
    ordered_set<const Constraints::SolitaryConstraint *> solitaryConstraints;
    ordered_set<const Constraints::NoPackConstraint *> nopackConstraints;
    ordered_set<const Constraints::NoOverlapConstraint *> nooverlapConstraints;
    ordered_set<const Constraints::MutuallyAlignedConstraint *> mutuallyAlignedConstraints;
    ordered_set<const Constraints::DeparsedToTMConstraint *> deparseToTMConstraints;
    ordered_set<const Constraints::NoSplitConstraint *> noSplitConstraints;
};

using FieldListEntry = std::pair<int, const IR::Type *>;
using AlignmentConstraint = Constraints::AlignmentConstraint;
using SolitaryConstraint = Constraints::SolitaryConstraint;
using RepackedHeaders = std::vector<std::pair<const IR::HeaderOrMetadata *, std::string>>;
using ExtractedTogether = ordered_map<cstring, ordered_set<cstring>>;
using PackingCandidateFields = ordered_map<cstring, ordered_set<cstring>>;
using PackedFields = ordered_map<cstring, std::vector<cstring>>;
using PackingConstraints = ordered_map<cstring, std::vector<z3::expr>>;

/**
 * Represents constraints induced in the ConstraintSolver class in the form
 * header -> constraint type -> description of constraint.
 */
using DebugInfo = ordered_map<cstring, ordered_map<cstring, ordered_set<cstring>>>;

/**
 * \ingroup bridged_packing
 * \brief Adjusted packing of bridged and fixed-size headers;
 *        output of BFN::BridgedPacking, input for BFN::SubstitutePackedHeaders.
 *
 * There are two sources of adjusted packing:
 * - PackWithConstraintSolver::solve
 * - PadFixedSizeHeaders::preorder
 *
 * Adjusted packing is then used in the ReplaceFlexibleType pass.
 */
using RepackedHeaderTypes = ordered_map<cstring, const IR::Type_StructLike *>;

/**
 * The inspector builds a map of aliased fields (IR::BFN::AliasMember) mapping
 * alias destinations to their sources. Only aliases with destinations annotated
 * with the \@flexible annotation are considered.
 *
 * <table>
 * <caption>Logging options</caption>
 * <tr>
 * <td>-Tbridged_packing:5
 * <td>
 * - The aliases being added to the map
 * - Notice when a field is marked bridged in PhvInfo and is not included
 *   in the CollectIngressBridgedFields::bridged_to_orig map
 * </table>
 *
 * @pre The Alias pass needs to be performed before this one to create
 *      IR::BFN::AliasMember IR nodes.
 *      TODO: We could figure out the mapping from def-use analysis instead.
 */
class CollectIngressBridgedFields : public Inspector {
 private:
    const PhvInfo &phv;

    bool preorder(const IR::BFN::Unit *unit) override {
        if (unit->thread() == EGRESS) return false;
        return true;
    }

    profile_t init_apply(const IR::Node *root) override;
    void postorder(const IR::BFN::AliasMember *mem) override;
    void end_apply() override;

 public:
    explicit CollectIngressBridgedFields(const PhvInfo &phv) : phv(phv) {}

    /// Key: bridged field name, value: original field name
    ordered_map<cstring, cstring> bridged_to_orig;
};

/**
 * The inspector builds the CollectEgressBridgedFields::bridged_to_orig map
 * mapping source (bridged) fields to the destination (original) fields.
 * Only non-solitary destination fields are considered
 * (those not used in an arithmetic operation).
 * Only flexible source fields are considered.
 *
 * The information is later used to induce alignment constraints.
 *
 * <table>
 * <caption>Logging options</caption>
 * <tr>
 * <td>-Tbridged_packing:5
 * <td>
 * - Candidate fields to be added to the CollectEgressBridgedFields::bridged_to_orig map.
 * </table>
 *
 * @pre The ParserCopyProp pass cannot be run before this inspector since
 *      it removes assignment statements in parser states.
 *      TODO Why assignments? This pass processes only extracts.
 */
class CollectEgressBridgedFields : public Inspector {
    const PhvInfo &phv;
    /// Map-map-set: destination (IR::BFN::ParserLVal) -> source (IR::BFN::SavedRVal) -> state
    ordered_map<const PHV::Field *,
                ordered_map<const PHV::Field *, ordered_set<const IR::BFN::ParserState *>>>
        candidateSourcesInParser;

 public:
    explicit CollectEgressBridgedFields(const PhvInfo &p) : phv(p) {}

    /// Map: bridged field name -> original field name
    ordered_map<cstring, cstring> bridged_to_orig;

    Visitor::profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::BFN::Extract *extract) override;
    void end_apply() override;
};

/**
 * The inspector collects all fields extracted in parser and stores both directions
 * of mapping between source (original) fields (IR::BFN::SavedRVal)
 * and bridged (destination) fields (IR::BFN::FieldLVal).
 *
 * The information is later used to induce alignment constraints.
 *
 * <table>
 * <caption>Logging options</caption>
 * <tr>
 * <td>-Tbridged_packing:5
 * <td>
 * - The fields being extracted to in parser
 * </table>
 */
class GatherParserExtracts : public Inspector {
 public:
    using FieldToFieldSet = ordered_map<const PHV::Field *, ordered_set<const PHV::Field *>>;

 private:
    const PhvInfo &phv;
    /// Extracted fields (IR::BFN::FieldLVal -> IR::BFN::SavedRVal)
    FieldToFieldSet parserAlignedFields;
    /// Extracted fields (IR::BFN::SavedRVal -> IR::BFN::FieldLVal)
    FieldToFieldSet reverseParserAlignMap;

    profile_t init_apply(const IR::Node *root) override {
        parserAlignedFields.clear();
        reverseParserAlignMap.clear();
        return Inspector::init_apply(root);
    }

    bool preorder(const IR::BFN::Extract *e) override;

 public:
    explicit GatherParserExtracts(const PhvInfo &p) : phv(p) {}

    bool count(const PHV::Field *f) const { return parserAlignedFields.count(f); }

    const ordered_set<const PHV::Field *> &at(const PHV::Field *f) const {
        static ordered_set<const PHV::Field *> emptySet;
        if (!parserAlignedFields.count(f)) return emptySet;
        return parserAlignedFields.at(f);
    }

    bool revCount(const PHV::Field *f) const { return reverseParserAlignMap.count(f); }

    const ordered_set<const PHV::Field *> &revAt(const PHV::Field *f) const {
        static ordered_set<const PHV::Field *> emptySet;
        if (!reverseParserAlignMap.count(f)) return emptySet;
        return reverseParserAlignMap.at(f);
    }

    const FieldToFieldSet &getAlignedMap() const { return parserAlignedFields; }
    const FieldToFieldSet &getReverseMap() const { return reverseParserAlignMap; }
};

template <typename NodeType, typename Func, typename Func2>
void forAllMatchingDoPreAndPostOrder(const IR::Node *root, Func &&function, Func2 &&function2) {
    struct NodeVisitor : public Inspector {
        explicit NodeVisitor(Func &&function, Func2 &&function2)
            : function(function), function2(function2) {}
        Func function;
        Func2 function2;
        bool preorder(const NodeType *node) override {
            function(node);
            return true;
        }
        void postorder(const NodeType *node) override { function2(node); }
    };
    root->apply(NodeVisitor(std::forward<Func>(function), std::forward<Func2>(function2)));
}

template <typename NodeType, typename Func>
void forAllMatchingDoPostOrder(const IR::Node *root, Func &&function) {
    struct NodeVisitor : public Inspector {
        explicit NodeVisitor(Func &&function) : function(function) {}
        Func function;
        void postorder(const NodeType *node) override { function(node); }
    };
    root->apply(NodeVisitor(std::forward<Func>(function)));
}

/**
 * The inspector collects all header/metadata/digest field list fields
 * with the \@flexible annotation and builds both directions of mapping
 * between digest field list fields and their source header/metadata fields.
 *
 * The information would be later used to induce alignment constraints.
 * **However, it is not currently being used anywhere.**
 *
 * <table>
 * <caption>Logging options</caption>
 * <tr>
 * <td>-Tbridged_packing:1
 * <td>
 * - The header/metadata/digest field list fields with the \@flexible annotation
 *   and header/metadata/digest field list type names
 * </table>
 */
class GatherDigestFields : public Inspector {
 public:
    using FieldToFieldSet = ordered_map<const PHV::Field *, ordered_set<const PHV::Field *>>;

 private:
    const PhvInfo &phv;
    ordered_map<cstring, ordered_set<std::vector<const PHV::Field *>>> headers;
    ordered_map<cstring, ordered_set<std::vector<const PHV::Field *>>> digests;
    FieldToFieldSet digestFieldMap;
    FieldToFieldSet reverseDigestFieldMap;

    /// Made private to make sure the pass is not used anywhere
    const FieldToFieldSet &getDigestMap() const { return digestFieldMap; }
    /// Made private to make sure the pass is not used anywhere
    const FieldToFieldSet &getReverseDigestMap() const { return reverseDigestFieldMap; }

 public:
    explicit GatherDigestFields(const PhvInfo &p) : phv(p) {}
    bool preorder(const IR::BFN::DigestFieldList *fl) override;
    bool preorder(const IR::HeaderOrMetadata *hdr) override;
    void end_apply() override;
};

/**
 * The inspector induces alignment constraints for bridged fields based on the information
 * collected in the CollectIngressBridgedFields, CollectEgressBridgedFields,
 * GatherParserExtracts, GatherDigestFields, and ActionPhvConstraints passes,
 * and updates PhvInfo.
 *
 * \par no_split and no_split_container_size constraints
 * The alignment constraint analysis may collect multiple alignment constraints
 * on a single bridged field. In this case, the bridge packing algorithm must
 * pick one of the alignment constraint as the final alignment for packing. If
 * the selected alignment constraint is derived from a source field that span
 * across byte boundary, then we generate additional no_split and
 * no_split_container_size constraint on the bridged field. E.g.,
 * if the alignment requirement is bit 6 with a field size of 6, then the
 * no_split constraint is 1 and no_split_container_size is round_up(6 + 6).
 */
class CollectConstraints : public Inspector {
    PhvInfo &phv;

    struct Constraints {
        // collected alignment constraints;
        ordered_map<const PHV::Field *, ordered_set<AlignmentConstraint>> alignment;
        // two field must have the same constraint, exact value to be determined by solver.
        ordered_map<const PHV::Field *, std::vector<const PHV::Field *>> mutualAlignment;

        ordered_set<std::pair<const PHV::Field *, const PHV::Field *>> mustPack;
        ordered_set<std::pair<const PHV::Field *, SolitaryConstraint::SolitaryReason>> noPack;
    };
    Constraints constraints;

    const CollectIngressBridgedFields &ingressBridgedFields;
    const CollectEgressBridgedFields &egressBridgedFields;
    const GatherParserExtracts &parserAlignedFields;
    const GatherDigestFields &digestFields;
    const ActionPhvConstraints &actionConstraints;

    // determine alignment constraint on a single field.
    cstring getOppositeGressFieldName(cstring);
    cstring getEgressFieldName(cstring);

    ordered_set<const PHV::Field *> findAllRelatedFields(const PHV::Field *f);
    ordered_set<const PHV::Field *> findAllReachingFields(const PHV::Field *f);
    ordered_set<const PHV::Field *> findAllSourcingFields(const PHV::Field *f);
    ordered_set<const PHV::Field *> findAllRelatedFieldsOfCopackedFields(const PHV::Field *f);
    void computeAlignmentConstraint(const ordered_set<const PHV::Field *> &, bool);

    bool mustPack(const ordered_set<const PHV::Field *> &, const ordered_set<const PHV::Field *> &,
                  const ordered_set<const IR::MAU::Action *> &);
    void computeMustPackConstraints(const ordered_set<const PHV::Field *> &, bool);

    void computeNoPackIfIntrinsicMeta(const ordered_set<const PHV::Field *> &, bool);
    void computeNoPackIfActionDataWrite(const ordered_set<const PHV::Field *> &, bool);
    void computeNoPackIfSpecialityRead(const ordered_set<const PHV::Field *> &, bool);
    void computeNoPackIfDigestUse(const ordered_set<const PHV::Field *> &, bool);
    void computeNoSplitConstraints(const ordered_set<const PHV::Field *> &, bool);

    bool preorder(const IR::HeaderOrMetadata *) override;
    bool preorder(const IR::BFN::DigestFieldList *) override;
    profile_t init_apply(const IR::Node *root) override;
    void end_apply() override;

    /**
     * Invoke an inspector @p function for every node of type @p NodeType in the
     * subtree rooted at @p root. The behavior is the same as a postorder
     * Inspector. Extended with a lambda for preorder function.
     */
    template <typename NodeType, typename Func, typename Func2>
    void forAllMatching(const IR::Node *root, Func &&function, Func2 &&function2) {
        struct NodeVisitor : public Inspector {
            explicit NodeVisitor(Func &&function, Func2 &&function2)
                : function(function), function2(function2) {}
            Func function;
            Func2 function2;
            bool preorder(const NodeType *node) override {
                function(node);
                return true;
            }
            void postorder(const NodeType *node) override { function2(node); }
        };
        root->apply(NodeVisitor(std::forward<Func>(function), std::forward<Func2>(function2)));
    }

 public:
    explicit CollectConstraints(PhvInfo &p, const CollectIngressBridgedFields &b,
                                const CollectEgressBridgedFields &e, const GatherParserExtracts &g,
                                const GatherDigestFields &d, const ActionPhvConstraints &a)
        : phv(p),
          ingressBridgedFields(b),
          egressBridgedFields(e),
          parserAlignedFields(g),
          digestFields(d),
          actionConstraints(a) {}
};

/**
 * @pre TODO: The Alias pass TODO why
 */
class GatherAlignmentConstraints : public PassManager {
 private:
    CollectIngressBridgedFields collectIngressBridgedFields;
    CollectEgressBridgedFields collectEgressBridgedFields;
    GatherParserExtracts collectParserAlignedFields;
    GatherDigestFields collectDigestFields;  // Outputs currently not used

 public:
    GatherAlignmentConstraints(PhvInfo &p, const ActionPhvConstraints &a)
        : collectIngressBridgedFields(p),
          collectEgressBridgedFields(p),
          collectParserAlignedFields(p),
          collectDigestFields(p) {
        addPasses({
            &collectIngressBridgedFields,
            &collectEgressBridgedFields,
            &collectParserAlignedFields,
            &collectDigestFields,
            new CollectConstraints(p, collectIngressBridgedFields, collectEgressBridgedFields,
                                   collectParserAlignedFields, collectDigestFields, a),
        });
    }
};

/**
 * \brief The class uses the Z3 solver to generate packing for a set of %PHV fields
 *        given a set of constraints.
 *
 * The constraints for the Z3 solver are determined using the information from PhvInfo
 * for the set of %PHV fields delivered using the ConstraintSolver::add_constraints
 * and ConstraintSolver::add_mutually_aligned_constraints methods.
 *
 * The method ConstraintSolver::solve executes the Z3 solver and it returns,
 * for each header, a vector of fields whose offset is adjusted based on the
 * solution of the Z3 solver with padding fields inserted where necessary.
 *
 * <table>
 * <caption>Logging options</caption>
 * <tr>
 * <td>-Tbridged_packing:1
 * <td>
 * - The fields the constraints are being added for to the Z3 solver
 * - The constraints being added to the Z3 solver
 * <tr>
 * <td>-Tbridged_packing:3
 * <td>
 * - The Z3 solver model in the case of a satisfied solution
 * - The Z3 solver core in the case of an unsatisfied solution
 * <tr>
 * <td>-Tbridged_packing:5
 * <td>
 * - The Z3 solver assertions
 * </table>
 *
 * @pre Up-to-date PhvInfo.
 */
class ConstraintSolver {
    const PhvInfo &phv;
    z3::context &context;
    z3::optimize &solver;
    PackingConstraints &packingConstraints;
    AllConstraints &constraints;
    /// Stores induced per-header-per-type constraints in a human-readable form.
    /// For use outside of this class.
    DebugInfo &debug_info;

    void add_field_alignment_constraints(cstring, const PHV::Field *, int);
    void add_non_overlap_constraints(cstring, ordered_set<const PHV::Field *> &);
    void add_extract_together_constraints(cstring, ordered_set<const PHV::Field *> &);
    void add_solitary_constraints(cstring, ordered_set<const PHV::Field *> &);
    void add_deparsed_to_tm_constraints(cstring, ordered_set<const PHV::Field *> &);
    void add_no_pack_constraints(cstring, ordered_set<const PHV::Field *> &);
    void add_no_split_constraints(cstring, ordered_set<const PHV::Field *> &);

    const PHV::Field *create_padding(int size);
    std::vector<const PHV::Field *> insert_padding(std::vector<std::pair<unsigned, std::string>> &);

    void dump_unsat_core();

 public:
    explicit ConstraintSolver(const PhvInfo &p, z3::context &context, z3::optimize &solver,
                              PackingConstraints &pc, AllConstraints &constraints, DebugInfo &dbg)
        : phv(p),
          context(context),
          solver(solver),
          packingConstraints(pc),
          constraints(constraints),
          debug_info(dbg) {}

    void add_constraints(cstring, ordered_set<const PHV::Field *> &);
    void add_mutually_aligned_constraints(ordered_set<const PHV::Field *> &);
    ordered_map<cstring, std::vector<const PHV::Field *>> solve(
        ordered_map<cstring, ordered_set<const PHV::Field *>> &);

    void print_assertions();
};

/**
 * The class adjusts the packing of headers/metadata/digest field lists
 * that contain the fields with the \@flexible annotation whose width is
 * not a multiply of eight bits.
 *
 * The candidates for packing adjustment can be delivered through the constructor.
 * This approach is used for bridged headers. If the list of candidates
 * (PackWithConstraintSolver::candidates) is empty, the inspector looks
 * for the candidates among all headers/metadata (IR::HeaderOrMetadata)
 * and digest field lists (IR::BFN::DigestFieldList).
 *
 * The class then computes the optimal field packing using the constraint solver
 * (PackWithConstraintSolver::solver).
 * We modeled as a single (larger) optimization problem as opposed to multiple
 * (smaller) optimization because a field may exists in multiple flexible field
 * lists. The computed alignment for the field must be consistent across all
 * instances of the flexible field list. However, the optimizer is still
 * optimizing each field list with individual optimization goals and
 * constraints.  The number of constraints scales linearly with respect to the
 * number of fields list instances. Within each field lists, the number of
 * constraints scales at O(n^2) with respect to the number of fields.
 *
 * <table>
 * <caption>Logging options</caption>
 * <tr>
 * <td>-Tbridged_packing:1
 * <td>
 * - A list of \@flexible fields whose width is not a multiple of eight bits
 * - Original (non-\@flexible) and adjusted (\@flexible whose width in not a multiply
 *   of eight bits) fields being used in the adjusted headers/metadata/digest field lists
 * - Adjusted headers/metadata/digest field lists
 * <tr>
 * <td>-Tbridged_packing:3
 * <td>
 * - Which digest field list is being processed
 * <tr>
 * <td>-Tbridged_packing:5
 * <td>
 * - Print candidates to pack and when a header/metadata/digest field list is skipped
 * </table>
 *
 * @pre Up-to-date PhvInfo.
 */
class PackWithConstraintSolver : public Inspector {
    const PhvInfo &phv;
    /// The constraint solver wrapping the Z3 solver to be used for adjusted packing
    /// of \@flexible fields whose width is not a multiply of eight bits
    ConstraintSolver &solver;
    /// A list of candidates to be processed.
    /// If empty, the candidates are found by this inspector.
    const ordered_set<cstring> &candidates;

    /// Adjusted packing
    ordered_map<cstring, const IR::Type_StructLike *> &repackedTypes;

    /// An ordered set of \@flexible fields whose width is not a multiply of eight bits
    /// for each header/metadata/digest field list
    ordered_map<cstring, ordered_set<const PHV::Field *>> nonByteAlignedFieldsMap;
    /// An ordered set of \@flexible fields whose width is a multiply of eight bits
    /// for each header/metadata/digest field list
    ordered_map<cstring, ordered_set<const PHV::Field *>> byteAlignedFieldsMap;
    /// A mapping from \@flexible PHV::Field to the corresponding IR::StructField
    /// for each header/metadata/digest field list
    // XXX: make this a ordered_map<cstring, IR::StructField>
    ordered_map<cstring, ordered_map<const PHV::Field *, const IR::StructField *>>
        phvFieldToStructFieldMap;

    /// FIXME: remove
    /// A mapping from a path such as ingress::hdr.foo to the corresponding
    /// struct field of hdr_t
    ordered_map<cstring, ordered_map<cstring, const IR::StructField *>> pathToStructFieldMap;

    PackingCandidateFields &packingCandidateFields;
    PackingConstraints &packingConstraints;

    /// A mapping from header/metadata/digest field list name to the corresponding
    /// IR::Type_StructLike for each header/metadata/digest field list
    ordered_map<cstring, const IR::Type_StructLike *> headerMap;

    // Pipeline name for printing debug messages
    cstring pipelineName;

 public:
    explicit PackWithConstraintSolver(const PhvInfo &p, ConstraintSolver &solver,
                                      const ordered_set<cstring> &c,
                                      ordered_map<cstring, const IR::Type_StructLike *> &r,
                                      PackingCandidateFields &pcf, PackingConstraints &pc)
        : phv(p),
          solver(solver),
          candidates(c),
          repackedTypes(r),
          packingCandidateFields(pcf),
          packingConstraints(pc) {}

    Visitor::profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::HeaderOrMetadata *hdr) override;
    bool preorder(const IR::BFN::DigestFieldList *dfl) override;
    void end_apply() override;

    void set_pipeline_name(cstring name) { pipelineName = name; }
    void optimize();
    void solve();
};

/**
 * The inspector looks for the phase0 and resubmit headers and adds their adjusted
 * versions to the PadFixedSizeHeaders::repackedTypes map. The adjusted versions
 * include a padding so that the headers have hardware-defined sizes.
 */
class PadFixedSizeHeaders : public Inspector {
    ordered_map<cstring, const IR::Type_StructLike *> &repackedTypes;

 public:
    explicit PadFixedSizeHeaders(ordered_map<cstring, const IR::Type_StructLike *> &r)
        : repackedTypes(r) {}

    bool preorder(const IR::BFN::Type_FixedSizeHeader *h) override {
        auto width = [&](const IR::Type_StructLike *s) -> int {
            int rv = 0;
            for (auto f : s->fields) rv += f->type->width_bits();
            return rv;
        };

        auto countPadding = [&](const IR::IndexedVector<IR::StructField> &fields) -> int {
            int count = 0;
            for (auto f = fields.begin(); f != fields.end(); f++) {
                if ((*f)->getAnnotation("padding"_cs)) count++;
            }
            return count;
        };

        auto genPadding = [&](int size, int id) {
            cstring padFieldName = "__pad_" + cstring::to_cstring(id);
            auto *fieldAnnotations =
                new IR::Annotations({new IR::Annotation(IR::ID("padding"), {}),
                                     new IR::Annotation(IR::ID("overlayable"), {})});
            const IR::StructField *padField =
                new IR::StructField(padFieldName, fieldAnnotations, IR::Type::Bits::get(size));
            return padField;
        };

        size_t bits = static_cast<size_t>(width(h));
        ERROR_CHECK(bits <= Device::pardeSpec().bitResubmitSize(), "%1% digest limited to %2% bits",
                    h->name, Device::pardeSpec().bitResubmitSize());
        auto pad_size = h->fixed_size - bits;

        auto fields = new IR::IndexedVector<IR::StructField>();
        fields->append(h->fields);
        auto index = countPadding(h->fields);
        if (pad_size != 0) {
            auto padding = genPadding(pad_size, index++);
            fields->push_back(padding);
        }

        auto newType = new IR::Type_Header(h->name, *fields);
        repackedTypes.emplace(h->name, newType);
        return false;
    }
};

/**
 * The transformer replaces specified headers/metadata/digest field lists
 * with adjusted versions. The adjusted versions are to be delivered through
 * the constructor (ReplaceFlexibleType::repackedTypes).
 *
 * The transformer works both with midend and backend IR.
 */
class ReplaceFlexibleType : public Transform {
    const RepackedHeaderTypes &repackedTypes;

 public:
    explicit ReplaceFlexibleType(const RepackedHeaderTypes &m) : repackedTypes(m) {}

    // if used in backend
    const IR::Node *postorder(IR::HeaderOrMetadata *h) override;
    const IR::Node *postorder(IR::BFN::DigestFieldList *d) override;
    const IR::BFN::DigestFieldList *repackFieldList(
        cstring digest, std::vector<FieldListEntry> repackedFieldIndices,
        const IR::Type_StructLike *repackedHeaderType,
        const IR::BFN::DigestFieldList *origFieldList) const;

    // if used in midend
    const IR::Node *postorder(IR::Type_StructLike *h) override;
    const IR::Node *postorder(IR::StructExpression *h) override;
};

bool findFlexibleAnnotation(const IR::Type_StructLike *);

/// This class gathers all the bridged metadata fields also used as deparser parameters. The
/// CollectPhvInfo pass sets the deparsed_bottom_bits() property for all deparser parameters to
/// true. Therefore, this alignment constraint needs to be recognized and respected during bridged
/// metadata packing.
class GatherDeparserParameters : public Inspector {
 private:
    const PhvInfo &phv;
    /// Set of detected deparser parameters.
    ordered_set<const PHV::Field *> &params;

    profile_t init_apply(const IR::Node *root) override {
        params.clear();
        return Inspector::init_apply(root);
    }

    bool preorder(const IR::BFN::DeparserParameter *p) override;

 public:
    explicit GatherDeparserParameters(const PhvInfo &p, ordered_set<const PHV::Field *> &f)
        : phv(p), params(f) {}
};

/// This class identifies all fields initialized in the parser during Phase 0. The output of this
/// pass is used later by RepackFlexHeaders as follows: If a bridged field is also initialized in
/// Phase 0, then that field is not packed with any other field.
class GatherPhase0Fields : public Inspector {
 private:
    const PhvInfo &phv;
    /// Set of all fields initialized in phase 0.
    ordered_set<const PHV::Field *> &noPackFields;
    static constexpr char const *PHASE0_PARSER_STATE_NAME = "ingress::$phase0";

    profile_t init_apply(const IR::Node *root) override {
        noPackFields.clear();
        return Inspector::init_apply(root);
    }

    bool preorder(const IR::BFN::ParserState *p) override;

    bool preorder(const IR::BFN::DigestFieldList *d) override;

 public:
    explicit GatherPhase0Fields(const PhvInfo &p, ordered_set<const PHV::Field *> &f)
        : phv(p), noPackFields(f) {}
};

// set of variables for bridged fields (pretty print)
// set of constraints for bridged fields
// helper function to convert between PHV::Field to z3::expr
// constraints are objects on PHV fields
// constraints can be pretty_printed, encoded to z3
class LogRepackedHeaders : public Inspector {
 private:
    // Need this for field names
    const PhvInfo &phv;
    // Contains all of the (potentially) repacked headers
    RepackedHeaders repacked;
    // All headers we have seen before, but with "egress" or "ingress" removed (avoid duplication)
    std::unordered_set<std::string> hdrs;

    // Collects all headers/metadatas that may have been repacked (i.e. have a field that is
    // flexible)
    bool preorder(const IR::HeaderOrMetadata *h) override;

    // Pretty print all of the flexible headers
    void end_apply() override;

    // Returns the full field name
    std::string getFieldName(std::string hdr, const IR::StructField *field) const;

    // Pretty prints a single header/metadata
    std::string pretty_print(const IR::HeaderOrMetadata *h, std::string hdr) const;

    // Strips the given prefix from the front of the cstring, returns as string
    std::string strip_prefix(cstring str, std::string pre);

 public:
    explicit LogRepackedHeaders(const PhvInfo &p) : phv(p) {}

    std::string asm_output() const;
};

class LogFlexiblePacking : public Logging::PassManager {
    LogRepackedHeaders *flexibleLogging;

 public:
    explicit LogFlexiblePacking(const PhvInfo &phv)
        : Logging::PassManager("flexible_packing"_cs, Logging::Mode::AUTO) {
        flexibleLogging = new LogRepackedHeaders(phv);
        addPasses({
            flexibleLogging,
        });
    }

    const LogRepackedHeaders *get_flexible_logging() const { return flexibleLogging; }
};

class GatherPackingConstraintFromSinglePipeline : public PassManager {
 private:
    const BFN_Options &options;
    MauBacktracker table_alloc;
    PragmaNoPack pa_no_pack;
    PackConflicts packConflicts;
    MapTablesToActions tableActionsMap;
    ActionPhvConstraints actionConstraints;
    SymBitMatrix doNotPack;
    TablesMutuallyExclusive tMutex;
    ActionMutuallyExclusive aMutex;
    ordered_set<const PHV::Field *> noPackFields;
    ordered_set<const PHV::Field *> deparserParams;

    // used by solver-based packing
    ordered_set<const PHV::Field *> fieldsToPack;
    PackWithConstraintSolver &packWithConstraintSolver;

 public:
    explicit GatherPackingConstraintFromSinglePipeline(PhvInfo &p, const PhvUse &u,
                                                       DependencyGraph &dg, const BFN_Options &o,
                                                       PackWithConstraintSolver &sol);

    // Return a Json representation of flexible headers to be saved in .bfa/context.json
    // must be called after the pass is applied
    std::string asm_output() const;
};

// PackFlexibleHeader manages the global context for packing bridge metadata
// The global context includes:
// - instance of z3 solver and its context
//
// We need to manage the global context at this level is because bridge
// metadata can be used across multiple pipelines in folded pipeline. To
// support folded pipeline, we first collect bridge metadata constraints across
// each pair of ingress/egress, and run z3 solver over the global set of
// constraints over all pairs of ingress&egress pipelines. Therefore, it
// is necessary to maintain z3 context as this level.
class PackFlexibleHeaders : public PassManager {
    std::vector<const IR::BFN::Pipe *> pipe;
    PhvInfo phv;
    PhvUse uses;
    FieldDefUse defuse;
    DependencyGraph deps;

    z3::context context;
    z3::optimize solver;
    ConstraintSolver constraint_solver;
    PackWithConstraintSolver packWithConstraintSolver;
    PackingCandidateFields packingCandidateFields;
    PackingConstraints packingConstraints;
    PackedFields packedFields;
    AllConstraints constraints;

    cstring pipelineName;
    /* storage for debug log */
    DebugInfo debug_info;

    GatherPackingConstraintFromSinglePipeline *flexiblePacking;

 public:
    explicit PackFlexibleHeaders(const BFN_Options &options, ordered_set<cstring> &,
                                 RepackedHeaderTypes &repackedTypes);

    void set_pipeline_name(cstring name) { pipelineName = name; }
    // encode constraints to z3 expressions
    void optimize() {
        packWithConstraintSolver.set_pipeline_name(pipelineName);
        packWithConstraintSolver.optimize();
    }
    // execute z3 solver to find a solution
    void solve() { packWithConstraintSolver.solve(); }
    // convert the solution to P4 header format
    void end_apply() override;

    void print_packing_candidate_fields() const {
        for (auto &[hdr_type, fields] : packingCandidateFields) {
            LOG3("Packing candidate fields for " << hdr_type << ":");
            for (auto f : fields) {
                LOG3("  " << f);
            }
        }
    }
    void check_conflicting_constraints();
};

/**
 * \defgroup bridged_packing Packing of bridged and fixed-size headers
 * \ingroup post_midend
 * \brief Overview of passes that adjust packing of bridged headers
 *
 * Packing of bridged headers is performed in order to satisfy constraints induced
 * by the processing threads where the bridged headers are used.
 * Bridging can be used from an ingress to an egress processing thread of a possibly
 * different pipe, from an egress to an ingress processing thread of the same pipe
 * in the case of a folded program, or the combination of both.
 *
 * The egress to ingress bridging is available on Tofino 32Q with loopbacks present
 * at pipes 1 and 3. It is not available on Tofino 64Q with no loopbacks.
 *
 * The case of the combination of both bridging in a folded program is shown below:
 *
 * \dot
 * digraph folded_pipeline {
 *   layout=neato
 *   node [shape=box,style="rounded,filled",fontname="sans-serif,bold",
 *     margin="0.05,0.05",color=dodgerblue3,fillcolor=dodgerblue3,fontcolor=white,fontsize=10]
 *   edge [fontname="sans-serif",color=gray40,fontcolor=gray40,fontsize=10]
 *   IG0 [label="pipe0:ingress", pos="-1,1!"]
 *   EG0 [label="pipe0:egress", pos="1,1!"]
 *   IG1 [label="pipe1:ingress", pos="-1,0!"]
 *   EG1 [label="pipe1:egress", pos="1,0!"]
 *   IG0 -> EG1
 *   EG1 -> IG1 [label="32Q loopback"]
 *   IG1 -> EG0
 * }
 * \enddot
 *
 * Packets are processed in a "logical" pipe(line) that spans multiple "physical" pipe(line)s.
 * Using the backend's terminology, the packet is processed by four threads
 * of the IR::BFN::Pipe::thread_t type in a run-to-completion manner.
 *
 * All bridged headers needs to be analyzed in all threads they are used in.
 * They are emitted by the source thread and extracted by the destination thread.
 * In the case above, if a bridged header is used in all threads, the analysis is
 * performed on pairs (pipe0:ingress, pipe1:egress), (pipe1:egress, pipe1:ingress),
 * and (pipe1:ingress, pipe0:egress) (following the arrows).
 *
 * Packing of bridged headers is performed only for the headers/structs
 * with the \@flexible annotation and for resubmit headers,
 * which are fixed-sized (transformed into the IR::BFN::Type_FixedSizeHeader type in midend).
 *
 * The steps are as follows:
 *
 * 1. BFN::BridgedPacking:
 *    Perform some auxiliary passes and convert the midend IR towards the backend IR
 *    (BackendConverter).
 * 2. BFN::BridgedPacking -> ExtractBridgeInfo::preorder(IR::P4Program):
 *    Find usages of bridged headers (CollectBridgedFieldsUse).
 * 3. BFN::BridgedPacking -> ExtractBridgeInfo::end_apply():
 *    For all pairs of gresses where the bridged headers are used, perform some auxiliary
 *      backend passes (under PackFlexibleHeaders) and collect the constraints of
 *      bridged headers (FlexiblePacking).
 * 4. BFN::BridgedPacking -> ExtractBridgeInfo::end_apply()
 *    -> PackFlexibleHeaders::solve() -> FlexiblePacking::solve()
 *    -> PackWithConstraintSolver::solve() -> ConstraintSolver::solve():
 *    Use Z3 solver to find a satisfying packing of the bridged headers,
 *      which is stored in the RepackedHeaderTypes map.
 * 5. BFN::BridgedPacking -> PadFixedSizeHeaders:
 *    Look for fixed-size headers and add their adjusted packing to the RepackedHeaderTypes map.
 * 6. The modifications of the auxiliary passes performed in 1., the converted backend IR,
 *      and the modifications of auxiliary backend passes performed in 3. are thrown away.
 * 7. BFN::SubstitutePackedHeaders -> ReplaceFlexibleType::postorder:
 *    The solution found by the Z3 solver is used to substitute the original bridged headers
 *      with adjusted ones. Fixed-size headers are also substituted with adjusted ones.
 * 8. BFN::SubstitutePackedHeaders:
 *    Perform the same auxiliary passes and convert the IR towards the backend IR as in 1.
 *      and perform the PostMidEndLast pass.
 */

using PipeAndGress = std::pair<std::pair<cstring, gress_t>, std::pair<cstring, gress_t>>;

using BridgeLocs = ordered_map<std::pair<cstring, gress_t>, IR::HeaderRef *>;

class CollectBridgedFieldsUse : public Inspector {
 public:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

    struct Use {
        const IR::Type *type;
        cstring name;
        cstring method;
        gress_t thread;

        bool operator<(const Use &other) const {
            return std::tie(type, name, method, thread) <
                   std::tie(other.type, other.name, other.method, other.thread);
        }

        bool operator==(const Use &other) const {
            return std::tie(type, name, method, thread) ==
                   std::tie(other.type, other.name, other.method, other.thread);
        }
    };

    ordered_set<Use> bridge_uses;

    friend std::ostream &operator<<(std::ostream &, const Use &u);

 public:
    CollectBridgedFieldsUse(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {}

    void postorder(const IR::MethodCallExpression *mc) override;
};

struct BridgeContext {
    int pipe_id;
    CollectBridgedFieldsUse::Use use;
    IR::BFN::Pipe::thread_t thread;
};

class ExtractBridgeInfo : public Inspector {
    const BFN_Options &options;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    BFN::BackendConverter *conv;
    ParamBinding *bindings;
    RepackedHeaderTypes &map;
    CollectGlobalPragma collect_pragma;

 public:
    ordered_map<int, ordered_set<CollectBridgedFieldsUse::Use>> all_uses;

    ExtractBridgeInfo(BFN_Options &options, P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                      BFN::BackendConverter *conv, ParamBinding *bindings, RepackedHeaderTypes &ht)
        : options(options),
          refMap(refMap),
          typeMap(typeMap),
          conv(conv),
          bindings(bindings),
          map(ht) {}

    std::vector<const IR::BFN::Pipe *> *generate_bridge_pairs(std::vector<BridgeContext> &);

    bool preorder(const IR::P4Program *program) override;
    void end_apply(const IR::Node *) override;
};

/**
 * \ingroup bridged_packing
 * \brief The pass analyzes the usage of bridged headers and adjusts their packing.
 *
 * @pre Apply this pass manager to IR::P4Program after midend processing.
 * @post The RepackedHeaderTypes map filled in with adjusted packing of bridged headers.
 */
class BridgedPacking : public PassManager {
    ParamBinding *bindings;
    BFN::ApplyEvaluator *evaluator;
    BFN::BackendConverter *conv;
    RepackedHeaderTypes &map;
    ExtractBridgeInfo *extractBridgeInfo;

 public:
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;

 public:
    BridgedPacking(BFN_Options &options, RepackedHeaderTypes &repackMap,
                   CollectSourceInfoLogging &sourceInfoLogging);

    IR::Vector<IR::BFN::Pipe> pipe;
    ordered_map<int, const IR::BFN::Pipe *> pipes;
};

/**
 * \ingroup bridged_packing
 * \brief The pass substitutes bridged headers with adjusted ones
 *        and converts the IR into the backend form.
 */
class SubstitutePackedHeaders : public PassManager {
    ParamBinding *bindings;
    BFN::ApplyEvaluator *evaluator;
    BFN::BackendConverter *conv;

 public:
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    IR::Vector<IR::BFN::Pipe> pipe;
    ordered_map<int, const IR::BFN::Pipe *> pipes;

 public:
    SubstitutePackedHeaders(BFN_Options &options, const RepackedHeaderTypes &repackedMap,
                            CollectSourceInfoLogging &sourceInfoLogging);
    const BFN::ProgramPipelines &getPipelines() const { return conv->getPipelines(); }
    const IR::ToplevelBlock *getToplevelBlock() const { return evaluator->toplevel; }
};

#endif /* BACKENDS_TOFINO_BF_P4C_COMMON_BRIDGED_PACKING_H_ */
