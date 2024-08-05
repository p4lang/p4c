#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_DECLARATIONS_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_DECLARATIONS_H_

#include <string>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/parser.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/common/table.h"
#include "backends/p4tools/modules/smith/core/target.h"
#include "backends/p4tools/modules/smith/util/util.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/source_file.h"

namespace P4Tools::P4Smith {

using namespace P4::literals;

class Bmv2V1ModelDeclarationGenerator : public DeclarationGenerator {
 public:
    explicit Bmv2V1ModelDeclarationGenerator(SmithTarget &parent) : DeclarationGenerator(parent) {}

    IR::ParameterList *genParameterList() override;
};

}  // namespace P4Tools::P4Smith

#endif  /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_DECLARATIONS_H_ */
