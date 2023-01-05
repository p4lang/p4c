#include "backends/p4tools/modules/testgen/targets/bmv2/p4_asserts_parser.h"

#include <stdint.h>

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include <boost/format.hpp>
#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/compiler/p4_expr_parser.h"
#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir-inline.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/big_int_util.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/ordered_map.h"
#include "lib/safe_vector.h"
#include "backends/p4tools/common/lib/formulae.h"

namespace P4Tools {

namespace ExpressionParser {

/// Convert access tokens or keys into table keys. For converting access tokens or keys to
/// table keys we must first use combineTokensToNames. Returns a vector with tokens converted into
/// table keys.
/// For example, at the input we have a vector of tokens:
/// [key::mask(Text), ==(Equal) , 0(Number)] The result will be [tableName_mask_key(Text), ==(Equal)
/// , 0(Number)] Other examples : [key::prefix_length(Text), ==(Equal) , 0(Number)] ->
/// [tableName_lpm_prefix_key(Text), ==(Equal) , 0(Number)] [key::priority(Text), ==(Equal) ,
/// 0(Number)] -> [tableName_priority_key(Text), ==(Equal) , 0(Number)] [Name01(Text), ==(Equal) ,
/// 0(Number)] -> [tableName_key_Name01(Text), ==(Equal) , 0(Number)]
class MemberToVariable : public Transform {
    cstring tableName;
    const std::map<cstring, const IR::Type*>& types;

 public:
    explicit MemberToVariable(cstring tableName, const std::map<cstring, const IR::Type*> types)
        : tableName(tableName), types(types) {};

    const IR::Node* preorder(IR::MethodCallExpression* methodCall) override {
        if (const auto* member = methodCall->method->to<IR::Member>()) {
            if (member->member.name == "isValid") {
                return Utils::getZombieConst(getType(member), 0,
                                             std::string(memberToString(member)));
            }
        }
        return methodCall;
    };

    const IR::Node* preorder(IR::Member* member) override {
        return Utils::getZombieConst(getType(member), 0,
                                     std::string(memberToString(member->expr)));
    }

 protected:
    cstring memberToString(const IR::Expression* expr) {
        cstring name;
        if (const auto* member = expr->to<IR::Member>()) {
            name = member->member.name;
            if (name == "mask" && member->type == nullptr) {
                name = toString(member->expr);
                return tableName + "_mask_" + name;
            }
            if (name == "prefix_length" && member->type == nullptr) {
                name = toString(member->expr);
                return tableName + "_lpm_prefix_" + name;
            }
            if (name == "priority" && member->type == nullptr) {
                name = toString(member->expr);
                return tableName + "_priority" + name;
            }
            if (name == "isValid") {
                name = toString(member->expr);
                return tableName + "_key_" + name;
            }
        }
        return tableName + "_key_" + toString(expr);
    }

    cstring toString(const IR::Expression* expr) {
        if (const auto* member = expr->to<IR::Member>()) {
            return toString(member->expr) + member->member.name;
        }
        if (const auto* pathExpression = expr->to<IR::PathExpression>()) {
            return pathExpression->path->name.name;
        }
        BUG("Unimplemented format of the name %1%", expr);
    }

    const IR::Type* getType(const IR::Member* member) {
        CHECK_NULL(member);
        auto name = member->member.name;
        auto typeName = types.find(name);
        if (typeName != types.end()) {
            return typeName->second;
        }
        if (name == "isValid") {
            return IR::Type_Bits::get(1);
        }
        if (name == "mask" || name == "prefix_length" || name == "priority") {
            return member->expr->type;
        }
        return member->type;
    }
};

AssertsParser::AssertsParser(std::vector<std::vector<const IR::Expression*>>& output)
    : restrictionsVec(output) {
    setName("Restrictions");
}

std::vector<const IR::Expression*> AssertsParser::genIRStructs(
    cstring tableName, cstring restrictionString, const IR::Vector<IR::KeyElement>& keyElements) {
    const auto* restr = ExpressionParser::Parser::getIR(restrictionString, p4Program, true);
    std::vector<const IR::Expression*> result;
    std::map<cstring, const IR::Type*> types;
    for (const auto* key : keyElements) {
        cstring keyName;
        if (const auto* annotation = key->getAnnotation(IR::Annotation::nameAnnotation)) {
            keyName = annotation->getName();
        }
        BUG_CHECK(keyName.size() > 0, "Key does not have a name annotation.");
        const auto* keyType = key->expression->type;
        const IR::Type* type = nullptr;
        if (const auto* bit = keyType->to<IR::Type_Bits>()) {
            type = bit;
        } else if (const auto* varbit = keyType->to<IR::Extracted_Varbits>()) {
            type = varbit;
        } else if (keyType->is<IR::Type_Boolean>()) {
            type = IR::Type_Bits::get(1);
        } else {
            BUG("Unexpected key type %s.", keyType->node_type_name());
        }
        types.emplace(keyName, type);
    }

    MemberToVariable memberToVariable(tableName, types);
    if (const auto* listExpression = restr->to<IR::ListExpression>()) {
        for (const auto* component : listExpression->components) {
            result.push_back(component->apply(memberToVariable)->to<IR::Expression>());
        }
    } else {
        result.push_back(restr->apply(memberToVariable)->to<IR::Expression>());
    }
    return result;
}

const IR::Node* AssertsParser::preorder(IR::P4Program* program) {
    p4Program = program;
    return program;
}

const IR::Node* AssertsParser::postorder(IR::P4Table* table) {
    const auto* annotation = table->getAnnotation("entry_restriction");
    const auto* key = table->getKey();
    if (annotation == nullptr || key == nullptr) {
        return table;
    }
    for (const auto* restrStr : annotation->body) {
        auto restrictions =
            genIRStructs(table->controlPlaneName(), restrStr->text, key->keyElements);
        /// Using Z3Solver, we check the feasibility of restrictions, if they are not
        /// feasible, we delete keys and entries from the table to execute
        /// default_action
        Z3Solver solver;
        if (solver.checkSat(restrictions) == true) {
            restrictionsVec.push_back(restrictions);
            continue;
        }
        auto* cloneTable = table->clone();
        auto* cloneProperties = table->properties->clone();
        IR::IndexedVector<IR::Property> properties;
        for (const auto* property : cloneProperties->properties) {
            if (property->name.name != "key" || property->name.name != "entries") {
                properties.push_back(property);
            }
        }
        cloneProperties->properties = properties;
        cloneTable->properties = cloneProperties;
        return cloneTable;
    }
    return table;
}


}  // namespace AssertsParser

}  // namespace P4Tools
