#ifndef TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_
#define TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_

#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ir/ir.h"

namespace P4Tools {
namespace RefersToParser {

class RefersToParser : public Inspector {
    std::vector<std::vector<const IR::Expression*>>& restrictionsVec;

    std::vector<const IR::P4Action*> actionVector;

    /// Finds a P4Action in the actionVector according
    /// to the specified input argument which is an ActionListElement
    const IR::P4Action* findAction(const IR::ActionListElement* input);

    /// An intermediate function that determines the type for future variables and partially
    /// collects their names for them, after which it calls the createConstraint function,
    /// which completes the construction of the constraint
    void createRefersToConstraint(const IR::Vector<IR::Annotation>& annotations,
                                  const IR::Type* inputType, cstring controlPlaneName, int id,
                                  bool isParameter, cstring inputName);

 public:
    explicit RefersToParser(std::vector<std::vector<const IR::Expression*>>& output);

    void postorder(const IR::ActionListElement* action) override;

    void postorder(const IR::Annotation* annotation) override;

    /// Builds names for the zombie constant and then creates a zombie constant and builds the
    /// refers_to constraints based on them
    void createConstraint(bool table, cstring currentName, cstring currentKeyName,
                          cstring destKeyName, cstring destTableName, const IR::Type* type);
};

}  // namespace RefersToParser
}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_BMV2_P4_REFERS_TO_PARSER_H_ */
