#include "backends/p4tools/modules/testgen/targets/bmv2/p4_refers_to_parser.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "backends/p4tools/common/lib/variables.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"

namespace P4Tools::RefersToParser {

RefersToParser::RefersToParser(std::vector<std::vector<const IR::Expression *>> &output)
    : restrictionsVec(output) {
    setName("RefersToParser");
}

const IR::SymbolicVariable *RefersToParser::buildReferredKey(const IR::P4Control &ctrlContext,
                                                             const IR::Annotation &refersAnno) {
    auto annotationList = refersAnno.body;
    BUG_CHECK(annotationList.size() > 2,
              "'@refers_to' annotation %1% does not have the correct format.", refersAnno);
    auto srcTableRefStr = annotationList.at(0)->text;
    // Build the referred key by assembling all remaining tokens.
    // E.g., "hdr.eth.eth_type" is multiple tokens.
    cstring referredKeyStr = "";
    for (uint64_t i = 2; i < annotationList.size(); i++) {
        referredKeyStr += annotationList[i]->text;
    }
    const IR::IDeclaration *srcTableRef = nullptr;
    for (const auto *decl : *ctrlContext.getDeclarations()) {
        auto declName = decl->controlPlaneName();
        if (declName.endsWith(srcTableRefStr)) {
            srcTableRef = decl;
            break;
        }
    }
    BUG_CHECK(srcTableRef != nullptr, "Table %1% does not exist.", srcTableRefStr);
    const auto *srcTable = srcTableRef->checkedTo<IR::P4Table>();
    const auto *key = srcTable->getKey();
    BUG_CHECK(key != nullptr, "Table %1% does not have any keys.", srcTable);
    for (const auto *keyElement : key->keyElements) {
        auto annotations = keyElement->annotations->annotations;
        const auto *nameAnnot = keyElement->getAnnotation("name");
        // Some hidden tables do not have any key name annotations.
        BUG_CHECK(nameAnnot != nullptr, "Refers-to table key without a name annotation");
        if (referredKeyStr == nameAnnot->getName()) {
            auto referredKeyName = srcTable->controlPlaneName() + "_key_" + referredKeyStr;
            return ToolsVariables::getSymbolicVariable(keyElement->expression->type,
                                                       referredKeyName);
        }
    }
    BUG("Did not find a matching key in table %1%. ", srcTable);
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
                BUG_CHECK(nameAnnot != nullptr, "refers_to table key without a name annotation");
                const auto *referredKey = buildReferredKey(*ctrl, *annotation);
                auto srcKeyName = table->controlPlaneName() + "_key_" + nameAnnot->getName();
                const auto *srcKey =
                    ToolsVariables::getSymbolicVariable(keyElement->expression->type, srcKeyName);
                auto *expr = new IR::Equ(srcKey, referredKey);
                std::vector<const IR::Expression *> constraint;
                constraint.push_back(expr);
                restrictionsVec.push_back(constraint);
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
                    const auto *referredKey = buildReferredKey(*ctrl, *annotation);
                    auto srcParamName = table->controlPlaneName() + "_arg_" +
                                        actionCall->controlPlaneName() + std::to_string(idx);
                    const auto *srcKey =
                        ToolsVariables::getSymbolicVariable(parameter->type, srcParamName);
                    auto *expr = new IR::Equ(srcKey, referredKey);
                    std::vector<const IR::Expression *> constraint;
                    constraint.push_back(expr);
                    restrictionsVec.push_back(constraint);
                }
            }
        }
    }
    return false;
}

}  // namespace P4Tools::RefersToParser
