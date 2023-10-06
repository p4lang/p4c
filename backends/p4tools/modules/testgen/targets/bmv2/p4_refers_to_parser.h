#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_

#include <vector>

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4Tools::RefersToParser {

class RefersToParser : public Inspector {
    std::vector<std::vector<const IR::Expression *>> &restrictionsVec;

    /// Build the referred table key by looking up the table referenced in the annotation @param
    /// refersAnno in the
    /// @param ctrlContext and retrieving the appropriate type. @returns a symbolic variable
    /// corresponding to the control plane entry variable.
    const IR::SymbolicVariable *buildReferredKey(const IR::P4Control &ctrlContext,
                                                 const IR::Annotation &refersAnno);

    bool preorder(const IR::P4Table *table) override;

 public:
    explicit RefersToParser(std::vector<std::vector<const IR::Expression *>> &output);
};

}  // namespace P4Tools::RefersToParser

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_ */
