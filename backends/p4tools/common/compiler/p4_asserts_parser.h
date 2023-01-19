#ifndef BACKENDS_P4TOOLS_COMMON_COMPILER_P4_ASSERTS_PARSER_H_
#define BACKENDS_P4TOOLS_COMMON_COMPILER_P4_ASSERTS_PARSER_H_

#include <cstddef>
#include <iterator>
#include <set>
#include <string_view>
#include <vector>

#include "ir/ir.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools {
namespace ExpressionParser {

class AssertsParser : public Transform {
    std::vector<std::vector<const IR::Expression*>>& restrictionsVec;
    const IR::P4Program* p4Program;

 public:
    explicit AssertsParser(std::vector<std::vector<const IR::Expression*>>& output);
    const IR::Node* preorder(IR::P4Program* program) override;
    const IR::Node* postorder(IR::P4Table* table) override;
 protected:
    /// A function that calls the beginning of the transformation of restrictions from string into
    /// an IR::Expression. Internally calls all other necessary functions, for example
    /// combineTokensToNames and the like, to eventually get an IR expression that meets
    /// the string constraint
    std::vector<const IR::Expression*> genIRStructs(
    cstring tableName, cstring restrictionString, const IR::Vector<IR::KeyElement>& keyElements);
};

}  // namespace ExpressionParser
}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_COMPILER_P4_ASSERTS_PARSER_H_ */
