#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_TABLE_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_TABLE_H_
#include <cstddef>

#include "backends/p4tools/modules/smith/common/generator.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace P4C::P4Tools::P4Smith {

class TableGenerator : public Generator {
 public:
    explicit TableGenerator(const SmithTarget &target) : Generator(target) {}

    virtual ~TableGenerator() = default;

    virtual IR::P4Table *genTableDeclaration();

    virtual IR::TableProperties *genTablePropertyList();

    virtual IR::KeyElement *genKeyElement(IR::ID match_kind);

    virtual IR::Key *genKeyElementList(size_t len);

    virtual IR::Property *genKeyProperty();

    virtual IR::MethodCallExpression *genTableActionCall(cstring method_name,
                                                         const IR::ParameterList &params);

    virtual IR::ActionList *genActionList(size_t len);

    IR::Property *genActionListProperty();
};

}  // namespace P4C::P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_TABLE_H_ */
