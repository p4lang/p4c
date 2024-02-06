#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_

#include "backends/p4tools/common/lib/variables.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4Tools::P4Testgen::Bmv2 {

class RefersToParser : public Inspector {
 private:
    /// A vector of restrictions imposed on the control-plane.
    ConstraintsVector restrictionsVector;

    /// A lookup map for builtin table keys.
    using RefersToBuiltinMap = std::map<cstring, std::map<cstring, IR::SymbolicVariable>>;
    static const RefersToBuiltinMap REFERS_TO_BUILTIN_MAP;

    /// Assemble the referenced key by concatenating all remaining tokens in the annotation.
    /// TODO: We should use the annotation parser for this.
    static cstring assembleKeyReference(const IR::Vector<IR::AnnotationToken> &annotationList,
                                        size_t offset);

    /// Lookup the key in the builtin map and return the corresponding symbolic variable.
    static const IR::SymbolicVariable *lookUpBuiltinKey(
        const IR::Annotation &refersAnno, const IR::Vector<IR::AnnotationToken> &annotationList);

    /// Lookup the key in the table and return the corresponding symbolic variable.
    static const IR::SymbolicVariable *lookUpKeyInTable(const IR::P4Table &srcTable,
                                                        cstring keyReference);

    /// Get the referred table key by looking up the table referenced in the annotation @param
    /// refersAnno in the
    /// @param ctrlContext and retrieving the appropriate type. @returns a symbolic variable
    /// corresponding to the control plane entry variable.
    static const IR::SymbolicVariable *getReferencedKey(const IR::P4Control &ctrlContext,
                                                        const IR::Annotation &refersAnno);

    bool preorder(const IR::P4Table *table) override;

 public:
    RefersToParser();

    /// Returns the restrictions imposed on the control-plane.
    [[nodiscard]] ConstraintsVector getRestrictionsVector() const;
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_ */
