#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_TABLE_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_TABLE_H_

#include "ir/ir.h"

namespace P4Tools::P4Smith {

class Table {
 public:
    virtual IR::P4Table *genTableDeclaration();

    virtual IR::TableProperties *genTablePropertyList();

    virtual IR::KeyElement *genKeyElement(cstring match_kind);

    virtual IR::Key *genKeyElementList(size_t len);

    virtual IR::Property *genKeyProperty();

    virtual IR::MethodCallExpression *genTableActionCall(cstring method_name,
                                                         IR::ParameterList params);

    virtual IR::ActionList *genActionList(size_t len);

    IR::Property *genActionListProperty();
};

}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_TABLE_H_ */
