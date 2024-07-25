#ifndef CONTROL_PLANE_ADDMISSINGIDS_H_
#define CONTROL_PLANE_ADDMISSINGIDS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"
#include "p4RuntimeSymbolTable.h"

namespace P4 {

class MissingIdAssigner : public Transform {
    /// The reference map. This is needed to identify the correct references for
    /// some extern constructs.
    ReferenceMap *refMap;

    /// The type map. This is needed to identify the correct types for some
    /// extern constructs.
    /// The typeMap is not constant because the flattenHeader pass inserts
    /// types.
    TypeMap *typeMap;

    /// Specifies the width of the IDs.
    static constexpr int ID_BIT_WIDTH = 32;

    /// The symbol table that contains all the ID information.
    /// This ID assigner first computes this table,
    /// then uses the results to assign missing IDs.
    const ControlPlaneAPI::P4RuntimeSymbolTable *symbols = nullptr;

    /// The arch builder is necessary to compute the correct symbol table for a
    /// particular architecture.
    const ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface &archBuilder;

    const IR::P4Program *preorder(IR::P4Program *program) override;
    const IR::Property *postorder(IR::Property *property) override;
    const IR::P4Table *postorder(IR::P4Table *table) override;
    const IR::Type_Header *postorder(IR::Type_Header *hdr) override;
    const IR::P4ValueSet *postorder(IR::P4ValueSet *valueSet) override;
    const IR::P4Action *postorder(IR::P4Action *action) override;

 public:
    explicit MissingIdAssigner(
        ReferenceMap *refMap, TypeMap *typeMap,
        const ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface &archBuilder);

    explicit MissingIdAssigner(
        ReferenceMap *refMap, TypeMap *typeMap,
        const ControlPlaneAPI::P4RuntimeSymbolTable *symbols,
        const ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface &archBuilder);
};

/// Scans the P4 program, run the evaluator pass, and derives the P4Runtime Ids
/// from the top level blocks produced by the evaluator pass. The pass then
/// computes IDs for P4Runtime nodes that are missing an ID and attaches the
/// computed ID to the node.
class AddMissingIdAnnotations final : public PassManager {
 public:
    AddMissingIdAnnotations(ReferenceMap *refMap, TypeMap *typeMap,
                            const ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface *archBuilder);
};

}  // namespace P4

#endif /* CONTROL_PLANE_ADDMISSINGIDS_H_ */
