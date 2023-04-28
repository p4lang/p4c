#include "backends/p4tools/modules/testgen/targets/bmv2/p4_refers_to_parser.h"

#include <stddef.h>

#include <cstdint>
#include <iostream>
#include <string>

#include "backends/p4tools/common/lib/variables.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"

namespace P4Tools::RefersToParser {

RefersToParser::RefersToParser(std::vector<std::vector<const IR::Expression *>> &output)
    : restrictionsVec(output) {
    setName("RefersToParser");
}

/// Builds names for the symbolic variable and then creates a symbolic variable and builds the
/// refers_to constraints based on them
void RefersToParser::createConstraint(bool table, cstring currentName, cstring currentKeyName,
                                      cstring destKeyName, cstring destTableName,
                                      const IR::Type *type) {
    cstring tmp = "";
    if (table) {
        tmp = currentName + "_key_" + currentKeyName;
    } else {
        tmp = currentName + currentKeyName;
    }
    auto left = ToolsVariables::getSymbolicVariable(type, 0, tmp);
    std::string str = currentName.c_str();
    std::vector<std::string> elems;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, '.')) {
        elems.push_back(item);
    }
    str = "";
    for (uint64_t i = 0; i < elems.size() - 1; i++) {
        str += elems[i] + ".";
    }
    tmp = str + destTableName + "_key_" + destKeyName;
    auto right = ToolsVariables::getSymbolicVariable(type, 0, tmp);
    auto *expr = new IR::Equ(left, right);
    std::vector<const IR::Expression *> constraint;
    constraint.push_back(expr);
    restrictionsVec.push_back(constraint);
}

// Builds a variable name from the body of the "refers_to" annotation.
// The build starts at index 2 because 0 is the table name and 1 is ",".
cstring buildName(IR::Vector<IR::AnnotationToken> input) {
    cstring result = "";
    for (uint64_t i = 2; i < input.size(); i++) {
        result += input[i]->text;
    }
    return result;
}

/// An intermediate function that determines the type for future variables and partially
/// collects their names for them, after which it calls the createConstraint function,
/// which completes the construction of the constraint
void RefersToParser::createRefersToConstraint(const IR::Annotation *annotation,
                                              const IR::Type *inputType, cstring controlPlaneName,
                                              bool isParameter, cstring inputName) {
    if (inputType->is<IR::Type_Boolean>()) {
        inputType = IR::Type_Bits::get(1);
    }
    createConstraint(!isParameter, controlPlaneName, inputName, buildName(annotation->body),
                     annotation->body[0]->text, inputType);
}

bool RefersToParser::preorder(const IR::P4Table *table) {
    const auto *key = table->getKey();
    if (key == nullptr) {
        return false;
    }
    const auto *ctrl = findOrigCtxt<IR::P4Control>();
    CHECK_NULL(ctrl);

    for (const auto *keyElement : key->keyElements) {
        auto annotations = keyElement->annotations->annotations;
        for (const auto *annotation : annotations) {
            if (annotation->name.name == "refers_to") {
                const auto *nameAnnot = keyElement->getAnnotation("name");
                // Some hidden tables do not have any key name annotations.
                BUG_CHECK(nameAnnot != nullptr, "Refers-to table key without a name annotation");
                cstring fieldName;
                if (nameAnnot != nullptr) {
                    fieldName = nameAnnot->getName();
                }
                createRefersToConstraint(annotation, keyElement->expression->type,
                                         table->controlPlaneName(), false, fieldName);
            }
        }
    }

    const auto *actionList = table->getActionList();
    if (actionList == nullptr) {
        return false;
    }
    for (const auto *action : actionList->actionList) {
        const auto *decl = ctrl->getDeclByName(action->getName().name);
        if (decl == nullptr) {
            return false;
        }
        const auto *actionCall = decl->checkedTo<IR::P4Action>();
        const auto *params = actionCall->parameters;
        if (params == nullptr) {
            return false;
        }
        for (size_t idx = 0; idx < params->parameters.size(); ++idx) {
            const auto *parameter = params->parameters.at(idx);
            auto annotations = parameter->annotations->annotations;
            for (const auto *annotation : annotations) {
                if (annotation->name.name == "refers_to") {
                    cstring inputName =
                        "_arg_" + actionCall->controlPlaneName() + std::to_string(idx);
                    createRefersToConstraint(annotation, parameter->type, table->controlPlaneName(),
                                             true, inputName);
                }
            }
        }
    }
    return false;
}

}  // namespace P4Tools::RefersToParser
