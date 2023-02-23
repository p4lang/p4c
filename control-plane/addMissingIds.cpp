#include "addMissingIds.h"

#include "frontends/p4/evaluator/evaluator.h"
#include "p4RuntimeAnnotations.h"
#include "p4RuntimeArchStandard.h"

namespace P4 {

const IR::P4Program *MissingIdAssigner::preorder(IR::P4Program *program) {
    auto evaluator = P4::EvaluatorPass(refMap, typeMap);
    const auto *newProg = program->apply(evaluator);
    auto *toplevel = evaluator.getToplevelBlock();
    CHECK_NULL(toplevel);
    symbols = ControlPlaneAPI::P4RuntimeSymbolTable::generateSymbols(
        toplevel->getProgram(), toplevel, refMap, typeMap, archBuilder(refMap, typeMap, toplevel));
    return newProg;
}

const IR::Property *MissingIdAssigner::postorder(IR::Property *property) {
    if (property->name != "key") {
        return property;
    }

    const auto *key = property->value->checkedTo<IR::Key>();
    if (key == nullptr) {
        return property;
    }

    auto *newKey = key->clone();
    IR::Vector<IR::KeyElement> newKeys;
    for (size_t symbolId = 0; symbolId < key->keyElements.size(); ++symbolId) {
        auto *keyElement = key->keyElements.at(symbolId)->clone();
        auto anno = ControlPlaneAPI::getIdAnnotation(keyElement);
        if (!anno) {
            const auto *annos = keyElement->getAnnotations();
            auto *newAnnos = annos->clone();
            IR::Vector<IR::Expression> annoExprs;
            const auto *idConst =
                new IR::Constant(new IR::Type_Bits(ID_BIT_WIDTH, false), symbolId + 1);
            annoExprs.push_back(idConst);
            newAnnos->add(new IR::Annotation("id", annoExprs));
            keyElement->annotations = newAnnos;
        }
        newKeys.push_back(keyElement);
    }
    newKey->keyElements = newKeys;
    auto *newProperty = property->clone();
    newProperty->value = newKey;
    return newProperty;
}

const IR::P4Table *MissingIdAssigner::postorder(IR::P4Table *table) {
    if (ControlPlaneAPI::isHidden(table)) {
        return table;
    }
    CHECK_NULL(symbols);
    auto anno = ControlPlaneAPI::getIdAnnotation(table);
    if (!anno) {
        const auto *annos = table->getAnnotations();
        auto *newAnnos = annos->clone();
        IR::Vector<IR::Expression> annoExprs;
        auto symbolId = symbols->getId(ControlPlaneAPI::P4RuntimeSymbolType::P4RT_TABLE(), table);
        const auto *idConst = new IR::Constant(new IR::Type_Bits(ID_BIT_WIDTH, false), symbolId);
        annoExprs.push_back(idConst);
        newAnnos->add(new IR::Annotation("id", annoExprs));
        table->annotations = newAnnos;
    }
    return table;
}

const IR::Type_Header *MissingIdAssigner::postorder(IR::Type_Header *hdr) {
    if (!ControlPlaneAPI::isControllerHeader(hdr) || ControlPlaneAPI::isHidden(hdr)) {
        return hdr;
    }
    CHECK_NULL(symbols);
    auto anno = ControlPlaneAPI::getIdAnnotation(hdr);
    if (!anno) {
        const auto *annos = hdr->getAnnotations();
        auto *newAnnos = annos->clone();
        IR::Vector<IR::Expression> annoExprs;
        auto symbolId =
            symbols->getId(ControlPlaneAPI::P4RuntimeSymbolType::P4RT_CONTROLLER_HEADER(), hdr);
        const auto *idConst = new IR::Constant(new IR::Type_Bits(ID_BIT_WIDTH, false), symbolId);
        annoExprs.push_back(idConst);
        newAnnos->add(new IR::Annotation("id", annoExprs));
        hdr->annotations = newAnnos;
    }

    return hdr;
}

const IR::P4ValueSet *MissingIdAssigner::postorder(IR::P4ValueSet *valueSet) {
    if (ControlPlaneAPI::isHidden(valueSet)) {
        return valueSet;
    }
    CHECK_NULL(symbols);
    auto anno = ControlPlaneAPI::getIdAnnotation(valueSet);
    if (!anno) {
        const auto *annos = valueSet->getAnnotations();
        auto *newAnnos = annos->clone();
        IR::Vector<IR::Expression> annoExprs;
        auto symbolId =
            symbols->getId(ControlPlaneAPI::P4RuntimeSymbolType::P4RT_VALUE_SET(), valueSet);
        const auto *idConst = new IR::Constant(new IR::Type_Bits(ID_BIT_WIDTH, false), symbolId);
        annoExprs.push_back(idConst);
        newAnnos->add(new IR::Annotation("id", annoExprs));
        valueSet->annotations = newAnnos;
    }
    return valueSet;
}

const IR::P4Action *MissingIdAssigner::postorder(IR::P4Action *action) {
    if (ControlPlaneAPI::isHidden(action)) {
        return action;
    }
    CHECK_NULL(symbols);
    auto anno = ControlPlaneAPI::getIdAnnotation(action);
    if (!anno) {
        const auto *annos = action->getAnnotations();
        auto *newAnnos = annos->clone();
        IR::Vector<IR::Expression> annoExprs;
        auto symbolId = symbols->getId(ControlPlaneAPI::P4RuntimeSymbolType::P4RT_ACTION(), action);
        const auto *idConst = new IR::Constant(new IR::Type_Bits(ID_BIT_WIDTH, false), symbolId);
        annoExprs.push_back(idConst);
        newAnnos->add(new IR::Annotation("id", annoExprs));
        action->annotations = newAnnos;
    }

    auto *paramList = new IR::ParameterList();
    for (size_t symbolId = 0; symbolId < action->parameters->size(); ++symbolId) {
        auto *param = action->parameters->getParameter(symbolId)->clone();
        auto anno = ControlPlaneAPI::getIdAnnotation(param);
        if (!anno) {
            const auto *annos = param->getAnnotations();
            auto *newAnnos = annos->clone();
            IR::Vector<IR::Expression> annoExprs;
            const auto *idConst =
                new IR::Constant(new IR::Type_Bits(ID_BIT_WIDTH, false), symbolId + 1);
            annoExprs.push_back(idConst);
            newAnnos->add(new IR::Annotation("id", annoExprs));
            param->annotations = newAnnos;
        }
        paramList->parameters.push_back(param);
    }
    action->parameters = paramList;
    return action;
}
MissingIdAssigner::MissingIdAssigner(
    ReferenceMap *refMap, TypeMap *typeMap,
    const ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface &archBuilder)
    : refMap(refMap), typeMap(typeMap), archBuilder(archBuilder) {
    CHECK_NULL(typeMap);
    CHECK_NULL(refMap);
    setName("MissingIdAssigner");
}

AddMissingIdAnnotations::AddMissingIdAnnotations(
    ReferenceMap *refMap, TypeMap *typeMap,
    const ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface *archBuilder) {
    CHECK_NULL(refMap);
    CHECK_NULL(typeMap);

    passes.push_back(new ControlPlaneAPI::ParseP4RuntimeAnnotations());
    passes.push_back(new MissingIdAssigner(refMap, typeMap, *archBuilder));
    setName("AddMissingIdAnnotations");
}

}  // namespace P4
