#ifndef BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_VARBITS_H_
#define BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_VARBITS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"
#include "lib/null.h"

namespace P4Tools {

/// Converts all existing Type_Varbit types in the program into a custom Extracted_Varbit type.
/// Extracted_Varbit also contains information about the width that was assigned to the type by
/// the extract call.
class ConvertVarbits : public Transform {
 public:
    ConvertVarbits();

    const IR::Node *postorder(IR::Type_Varbits *varbit) override;

    const IR::Node *postorder(IR::Expression *expr) override;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_VARBITS_H_ */
