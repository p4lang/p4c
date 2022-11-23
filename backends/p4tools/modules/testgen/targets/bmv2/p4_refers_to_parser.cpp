#include "backends/p4tools/modules/testgen/targets/bmv2/p4_refers_to_parser.h"

#include <stdint.h>

#include <algorithm>
#include <iostream>
#include <string>

#include "backends/p4tools/common/lib/util.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "lib/exceptions.h"
#include "lib/null.h"
#include "lib/safe_vector.h"
#include "p4tools/common/lib/formulae.h"

namespace P4Tools {

namespace RefersToParser {

RefersToParser::RefersToParser(std::vector<std::vector<const IR::Expression*>>& output)
    : restrictionsVec(output) {
    setName("RefersToParser");
}

/// Builds names for the zombie constant and then creates a zombie constant and builds the refers_to
/// constraints based on them
void RefersToParser::createConstraint(bool table, cstring currentName, cstring currentKeyName,
                                      cstring destKeyName, cstring destTableName,
                                      const IR::Type* type) {
    cstring tmp = "";
    if (table) {
        tmp = currentName + "_key_" + currentKeyName;
    } else {
        tmp = currentName + currentKeyName;
    }
    auto left = Utils::getZombieConst(type, 0, tmp);
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
    auto right = Utils::getZombieConst(type, 0, tmp);
    auto* expr = new IR::Equ(left, right);
    std::vector<const IR::Expression*> constraint;
    constraint.push_back(expr);
    restrictionsVec.push_back(constraint);
}

void RefersToParser::postorder(const IR::Annotation* annotation) {
    const IR::P4Action* prevAction = nullptr;
    if (annotation->name.name != "refers_to") {
        return;
    }
    if (const auto* action = findOrigCtxt<IR::P4Action>()) {
        if (prevAction != action) {
            actionVector.push_back(action);
            prevAction = action;
        }
    } else if (const auto* keys = findOrigCtxt<IR::Key>()) {
        const auto* table = findOrigCtxt<IR::P4Table>();
        CHECK_NULL(table);
        const auto* key = findOrigCtxt<IR::KeyElement>();
        CHECK_NULL(key);
        auto it = find(keys->keyElements.begin(), keys->keyElements.end(), key);
        if (it != keys->keyElements.end()) {
            int id = it - keys->keyElements.begin();
            createRefersToConstraint(key->annotations->annotations, key->expression->type,
                                     table->controlPlaneName(), id, false,
                                     key->expression->toString());
        }
    } else {
        BUG("refers_to annotation %1% is attached to unsupported element.", *annotation);
    }
}

// Finds a P4Action in the actionVector according
/// to the specified input argument which is an ActionListElement
const IR::P4Action* RefersToParser::findAction(const IR::ActionListElement* input) {
    for (const auto* element : actionVector) {
        if (input->getName().name == element->name.name) {
            return element;
        }
    }
    return nullptr;
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
void RefersToParser::createRefersToConstraint(const IR::Vector<IR::Annotation>& annotations,
                                              const IR::Type* inputType, cstring controlPlaneName,
                                              int id, bool isParameter, cstring inputName) {
    cstring currentKeyName = inputName;
    const IR::Type* type = nullptr;
    for (const auto* annotation : annotations) {
        if (annotation->name.name == "refers_to") {
            cstring destTableName = annotation->body[0]->text;
            cstring destKeyName = buildName(annotation->body);
            if (const auto* bit = inputType->to<IR::Type_Bits>()) {
                type = bit;
            } else if (const auto* varbit = inputType->to<IR::Extracted_Varbits>()) {
                type = varbit;
            } else if (inputType->is<IR::Type_Boolean>()) {
                type = IR::Type_Bits::get(1);
            } else {
                BUG("Unexpected key type %s.", inputType->node_type_name());
            }
            if (isParameter) {
                currentKeyName = "_arg_" + inputName + std::to_string(id);
                createConstraint(false, controlPlaneName, currentKeyName, destKeyName,
                                 destTableName, type);
            } else {
                createConstraint(true, controlPlaneName, currentKeyName, destKeyName, destTableName,
                                 type);
            }
        }
    }
}

void RefersToParser::postorder(const IR::ActionListElement* action) {
    const auto* actionCall = findAction(action);
    if (actionCall == nullptr) {
        return;
    }
    if (const auto* table = findOrigCtxt<IR::P4Table>()) {
        if (actionCall->parameters != nullptr) {
            int id = 0;
            for (const auto* parameter : actionCall->parameters->parameters) {
                if (parameter->annotations != nullptr) {
                    createRefersToConstraint(parameter->annotations->annotations, parameter->type,
                                             table->controlPlaneName(), id, true,
                                             actionCall->controlPlaneName());
                }
                id++;
            }
        }
    }
}
}  // namespace RefersToParser

}  // namespace P4Tools
