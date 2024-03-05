#include "backends/p4tools/modules/testgen/targets/bmv2/p4_refers_to_parser.h"

#include <cstddef>
#include <sstream>

#include "backends/p4tools/common/control_plane/symbolic_variables.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/exceptions.h"
#include "lib/null.h"

namespace P4Tools::P4Testgen::Bmv2 {

RefersToParser::RefersToParser() { setName("RefersToParser"); }

const RefersToParser::RefersToBuiltinMap RefersToParser::REFERS_TO_BUILTIN_MAP = {{
    "multicast_group_table",
    {
        {
            "multicast_group_id",
            IR::SymbolicVariable(IR::getBitType(16), "refers_to_multicast_group_id"),
        },
        {
            "replica.port",
            IR::SymbolicVariable(IR::getBitType(9), "refers_to_replica.port"),
        },
        {
            "replica.instance",
            IR::SymbolicVariable(IR::getBitType(16), "refers_to_replica.instance"),
        },
    },
}};

cstring RefersToParser::assembleKeyReference(const IR::Vector<IR::AnnotationToken> &annotationList,
                                             size_t offset) {
    // E.g., "hdr.eth.eth_type" is multiple tokens.
    std::stringstream keyReference;
    for (; offset < annotationList.size(); offset++) {
        keyReference << annotationList[offset]->text;
    }
    return keyReference;
}

const IR::SymbolicVariable *RefersToParser::lookUpBuiltinKey(
    const IR::Annotation &refersAnno, const IR::Vector<IR::AnnotationToken> &annotationList) {
    BUG_CHECK(
        annotationList.size() > 3,
        "'@refers_to' annotation %1% with \"builtin\" prefix does not have the correct format.",
        refersAnno);
    BUG_CHECK(annotationList.at(1)->text == ":" && annotationList.at(2)->text == ":",
              "'@refers_to' annotation %1% does not have the correct format.", refersAnno);
    cstring tableReference = "";
    size_t offset = 3;
    for (; offset < annotationList.size(); offset++) {
        auto token = annotationList[offset]->text;
        if (token == ",") {
            offset++;
            break;
        }
        tableReference += token;
    }
    cstring keyReference = assembleKeyReference(annotationList, offset);

    auto it = REFERS_TO_BUILTIN_MAP.find(tableReference);
    BUG_CHECK(it != REFERS_TO_BUILTIN_MAP.end(), "Unknown table %1%", tableReference);
    auto referredKey = it->second.find(keyReference);
    BUG_CHECK(referredKey != it->second.end(), "Unknown key %1% in table %2%", keyReference,
              tableReference);
    return &referredKey->second;
}

const IR::SymbolicVariable *RefersToParser::lookUpKeyInTable(const IR::P4Table &srcTable,
                                                             cstring keyReference) {
    const auto *key = srcTable.getKey();
    BUG_CHECK(key != nullptr, "Table %1% does not have any keys.", srcTable);
    for (const auto *keyElement : key->keyElements) {
        auto annotations = keyElement->annotations->annotations;
        const auto *nameAnnot = keyElement->getAnnotation("name");
        // Some hidden tables do not have any key name annotations.
        BUG_CHECK(nameAnnot != nullptr, "Refers-to table key without a name annotation");
        if (keyReference == nameAnnot->getName()) {
            return ControlPlaneState::getTableKey(srcTable.controlPlaneName(), keyReference,
                                                  keyElement->expression->type);
        }
    }
    BUG("Did not find a matching key in table %1%. ", srcTable);
}

const IR::SymbolicVariable *RefersToParser::getReferencedKey(const IR::P4Control &ctrlContext,
                                                             const IR::Annotation &refersAnno) {
    const auto &annotationList = refersAnno.body;
    BUG_CHECK(annotationList.size() > 2,
              "'@refers_to' annotation %1% does not have the correct format.", refersAnno);

    auto tableReference = annotationList.at(0)->text;
    if (tableReference == "builtin") {
        return lookUpBuiltinKey(refersAnno, annotationList);
    }
    BUG_CHECK(annotationList.at(1)->text == ",",
              "'@refers_to' annotation %1% does not have the correct format.", refersAnno);

    // Try to find the table the control declarations.
    // TODO: Currently this lookUp does not support aliasing and simply tries to find the first
    // occurrence of a table where the suffix matches.
    // Ideally, we would use originalName, but originalName currently is not preserved correctly.
    const IR::IDeclaration *tableDeclaration = nullptr;
    for (const auto *decl : *ctrlContext.getDeclarations()) {
        auto declName = decl->controlPlaneName();
        if (declName.endsWith(tableReference)) {
            tableDeclaration = decl;
            break;
        }
    }
    BUG_CHECK(tableDeclaration != nullptr, "Table %1% does not exist.", tableReference);
    const auto *srcTable = tableDeclaration->checkedTo<IR::P4Table>();

    cstring keyReference = assembleKeyReference(annotationList, 2);
    return lookUpKeyInTable(*srcTable, keyReference);
}

bool RefersToParser::preorder(const IR::P4Table *tableContext) {
    const auto *key = tableContext->getKey();
    if (key == nullptr) {
        return false;
    }
    const auto *controlContext = findOrigCtxt<IR::P4Control>();
    CHECK_NULL(controlContext);

    for (const auto *keyElement : key->keyElements) {
        auto annotations = keyElement->annotations->annotations;
        for (const auto *annotation : annotations) {
            if (annotation->name.name == "refers_to" || annotation->name.name == "referenced_by") {
                const auto *nameAnnot = keyElement->getAnnotation("name");
                BUG_CHECK(nameAnnot != nullptr, "%1% table key without a name annotation",
                          annotation->name.name);
                const auto *srcKey = ControlPlaneState::getTableKey(
                    tableContext->controlPlaneName(), nameAnnot->getName(),
                    keyElement->expression->type);
                const auto *referredKey = getReferencedKey(*controlContext, *annotation);
                restrictionsVector.push_back(new IR::Equ(srcKey, referredKey));
            }
        }
    }

    const auto *actionList = tableContext->getActionList();
    if (actionList == nullptr) {
        return false;
    }
    for (const auto *action : actionList->actionList) {
        const auto *decl = controlContext->getDeclByName(action->getName().name);
        if (decl == nullptr) {
            return false;
        }
        const auto *actionCall = decl->checkedTo<IR::P4Action>();
        const auto *params = actionCall->parameters;
        if (params == nullptr) {
            return false;
        }
        for (const auto *parameter : params->parameters) {
            auto annotations = parameter->annotations->annotations;
            for (const auto *annotation : annotations) {
                if (annotation->name.name == "refers_to" ||
                    annotation->name.name == "referenced_by") {
                    const auto *referredKey = getReferencedKey(*controlContext, *annotation);
                    const auto *srcKey = ControlPlaneState::getTableActionArgument(
                        tableContext->controlPlaneName(), actionCall->controlPlaneName(),
                        parameter->name, parameter->type);
                    auto *constraint = new IR::Equ(srcKey, referredKey);
                    restrictionsVector.push_back(constraint);
                }
            }
        }
    }
    return false;
}

ConstraintsVector RefersToParser::getRestrictionsVector() const { return restrictionsVector; }

}  // namespace P4Tools::P4Testgen::Bmv2
