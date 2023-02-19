#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_

#include <vector>

#include "ir/ir.h"
#include "ir/vector.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools::RefersToParser {

class RefersToParser : public Inspector {
    std::vector<std::vector<const IR::Expression *>> &restrictionsVec;

    /// An intermediate function that determines the type for future variables and partially
    /// collects their names for them, after which it calls the createConstraint function,
    /// which completes the construction of the constraint
    void createRefersToConstraint(const IR::Annotation *annotation, const IR::Type *inputType,
                                  cstring controlPlaneName, bool isParameter, cstring inputName);

 public:
    explicit RefersToParser(std::vector<std::vector<const IR::Expression *>> &output);

    bool preorder(const IR::P4Table *table) override;

    /// Builds names for the zombie constant and then creates a zombie constant and builds the
    /// refers_to constraints based on them
    void createConstraint(bool table, cstring currentName, cstring currentKeyName,
                          cstring destKeyName, cstring destTableName, const IR::Type *type);
};

}  // namespace P4Tools::RefersToParser

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_ */
