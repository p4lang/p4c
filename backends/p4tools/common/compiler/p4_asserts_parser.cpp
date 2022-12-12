#include "backends/p4tools/common/compiler/p4_asserts_parser.h"

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
#include "p4tools/common/lib/formulae.h"

namespace P4Tools {

namespace ExpressionParser {

AssertsParser::AssertsParser(std::vector<std::vector<const IR::Expression*>>& output)
    : restrictionsVec(output) {
    setName("Restrictions");
}
/// Convert access tokens or keys into table keys. For converting access tokens or keys to
/// table keys we must first use combineTokensToNames. Returns a vector with tokens converted into
/// table keys.
/// For example, at the input we have a vector of tokens:
/// [key::mask(Text), ==(Equal) , 0(Number)] The result will be [tableName_mask_key(Text), ==(Equal)
/// , 0(Number)] Other examples : [key::prefix_length(Text), ==(Equal) , 0(Number)] ->
/// [tableName_lpm_prefix_key(Text), ==(Equal) , 0(Number)] [key::priority(Text), ==(Equal) ,
/// 0(Number)] -> [tableName_priority_key(Text), ==(Equal) , 0(Number)] [Name01(Text), ==(Equal) ,
/// 0(Number)] -> [tableName_key_Name01(Text), ==(Equal) , 0(Number)]
std::vector<Token> combineTokensToTableKeys(std::vector<Token> input, cstring tableName) {
    std::vector<Token> result;
    for (uint64_t idx = 0; idx < input.size(); idx++) {
        if (!input[idx].is(Token::Kind::Text)) {
            result.push_back(input[idx]);
            continue;
        }
        auto str = std::string(input[idx].lexeme());

        auto substr = str.substr(0, str.find("::mask"));
        if (substr != str) {
            cstring cstr = tableName + "_mask_" + substr;
            result.emplace_back(Token::Kind::Text, cstr, cstr.size());
            continue;
        }
        substr = str.substr(0, str.find("::prefix_length"));
        if (substr != str) {
            cstring cstr = tableName + "_lpm_prefix_" + substr;
            result.emplace_back(Token::Kind::Text, cstr, cstr.size());
            continue;
        }

        substr = str.substr(0, str.find("::priority"));
        if (substr != str) {
            cstring cstr = tableName + "_priority";
            result.emplace_back(Token::Kind::Priority, cstr, cstr.size());
            continue;
        }

        if (str.find("isValid") != std::string::npos) {
            cstring cstr = tableName + "_key_" + str;
            result.emplace_back(Token::Kind::Text, cstr, cstr.size());
            idx += 2;
            continue;
        }

        cstring cstr = tableName + "_key_" + str;
        result.emplace_back(Token::Kind::Text, cstr, cstr.size());
    }
    return result;
}

/// A function that calls the beginning of the transformation of restrictions from a string into an
/// IR::Expression. Internally calls all other necessary functions, for example combineTokensToNames
/// and the like, to eventually get an IR expression that meets the string constraint
std::vector<const IR::Expression*> AssertsParser::genIRStructs(
    cstring tableName, cstring restrictionString, const IR::Vector<IR::KeyElement>& keyElements) {
    std::vector<const IR::Expression*> result;
    return result;
}


const IR::Node* AssertsParser::postorder(IR::P4Table* node) {
    const auto* annotation = node->getAnnotation("entry_restriction");
    const auto* key = node->getKey();
    if (annotation == nullptr || key == nullptr) {
        return node;
    }

    for (const auto* restrStr : annotation->body) {
        auto restrictions =
            genIRStructs(node->controlPlaneName(), restrStr->text, key->keyElements);
        /// Using Z3Solver, we check the feasibility of restrictions, if they are not
        /// feasible, we delete keys and entries from the table to execute
        /// default_action
        Z3Solver solver;
        if (solver.checkSat(restrictions) == true) {
            restrictionsVec.push_back(restrictions);
            continue;
        }
        auto* cloneTable = node->clone();
        auto* cloneProperties = node->properties->clone();
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
    return node;
}


}  // namespace AssertsParser

}  // namespace P4Tools
