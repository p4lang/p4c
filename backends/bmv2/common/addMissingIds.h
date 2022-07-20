
#ifndef BACKENDS_BMV2_COMMON_ADDMISSINGIDS_H_
#define BACKENDS_BMV2_COMMON_ADDMISSINGIDS_H_

#include "common/resolveReferences/referenceMap.h"
#include "control-plane/p4RuntimeAnnotations.h"
#include "control-plane/p4RuntimeArchStandard.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "control-plane/p4RuntimeSymbolTable.h"
#include "ir/ir.h"
#include "p4/evaluator/evaluator.h"
#include "p4/typeMap.h"

namespace P4 {

namespace BMV2 {

class MissingIdAssigner : public Transform {
    ReferenceMap* refMap;

    TypeMap* typeMap;

    ControlPlaneAPI::P4RuntimeSymbolTable symbols;

    const IR::P4Table* preorder(IR::P4Table* table) override {
        if (ControlPlaneAPI::isHidden(table)) {
            return table;
        }
        auto anno = ControlPlaneAPI::getIdAnnotation(table);
        if (!anno) {
            const auto* annos = table->getAnnotations();
            auto* newAnnos = annos->clone();
            IR::Vector<IR::Expression> annoExprs;
            auto symbolId = symbols.getId(
                ControlPlaneAPI::P4RuntimeSymbolType::TABLE(), table);
            const auto* idConst =
                new IR::Constant(new IR::Type_Bits(32, false), symbolId);
            annoExprs.push_back(idConst);
            newAnnos->add(new IR::Annotation("id", annoExprs));
            table->annotations = newAnnos;
        }
        int propertyIdx = 0;

        const IR::Key* key = nullptr;
        for (propertyIdx = 0;
             propertyIdx <
             static_cast<int>(table->properties->properties.size());
             ++propertyIdx) {
            const auto* property =
                table->properties->properties.at(propertyIdx);
            if (property->name == "key") {
                key = property->value->checkedTo<IR::Key>();
                break;
            }
        }
        if (key == nullptr) {
            return table;
        }
        auto* newKey = key->clone();
        IR::Vector<IR::KeyElement> newKeys;
        for (int symbolId = 0; symbolId < key->keyElements.size(); ++symbolId) {
            auto* keyElement = key->keyElements.at(symbolId)->clone();
            auto anno = ControlPlaneAPI::getIdAnnotation(keyElement);
            if (!anno) {
                const auto* annos = keyElement->getAnnotations();
                auto* newAnnos = annos->clone();
                IR::Vector<IR::Expression> annoExprs;
                const auto* idConst = new IR::Constant(
                    new IR::Type_Bits(32, false), symbolId + 1);
                annoExprs.push_back(idConst);
                newAnnos->add(new IR::Annotation("id", annoExprs));
                keyElement->annotations = newAnnos;
            }
            newKeys.push_back(keyElement);
        }
        newKey->keyElements = newKeys;
        auto* clonedTableProperties = table->properties->clone();
        auto* properties = &clonedTableProperties->properties;
        auto* propertyValue = properties->at(propertyIdx)->clone();
        propertyValue->value = newKey;
        (*properties)[propertyIdx] = propertyValue;
        table->properties = clonedTableProperties;

        return table;
    }

    const IR::Type_Header* preorder(IR::Type_Header* hdr) override {
        if (!ControlPlaneAPI::isControllerHeader(hdr) ||
            ControlPlaneAPI::isHidden(hdr)) {
            return hdr;
        }
        auto anno = ControlPlaneAPI::getIdAnnotation(hdr);
        if (!anno) {
            const auto* annos = hdr->getAnnotations();
            auto* newAnnos = annos->clone();
            IR::Vector<IR::Expression> annoExprs;
            auto symbolId = symbols.getId(
                ControlPlaneAPI::P4RuntimeSymbolType::CONTROLLER_HEADER(), hdr);
            const auto* idConst =
                new IR::Constant(new IR::Type_Bits(32, false), symbolId);
            annoExprs.push_back(idConst);
            newAnnos->add(new IR::Annotation("id", annoExprs));
            hdr->annotations = newAnnos;
        }

        return hdr;
    }

    const IR::P4ValueSet* preorder(IR::P4ValueSet* valueSet) override {
        if (ControlPlaneAPI::isHidden(valueSet)) {
            return valueSet;
        }
        auto anno = ControlPlaneAPI::getIdAnnotation(valueSet);
        if (!anno) {
            const auto* annos = valueSet->getAnnotations();
            auto* newAnnos = annos->clone();
            IR::Vector<IR::Expression> annoExprs;
            auto symbolId = symbols.getId(
                ControlPlaneAPI::P4RuntimeSymbolType::VALUE_SET(), valueSet);
            const auto* idConst =
                new IR::Constant(new IR::Type_Bits(32, false), symbolId);
            annoExprs.push_back(idConst);
            newAnnos->add(new IR::Annotation("id", annoExprs));
            valueSet->annotations = newAnnos;
        }
        return valueSet;
    }

    const IR::P4Action* preorder(IR::P4Action* action) override {
        if (ControlPlaneAPI::isHidden(action)) {
            return action;
        }
        auto anno = ControlPlaneAPI::getIdAnnotation(action);
        if (!anno) {
            const auto* annos = action->getAnnotations();
            auto* newAnnos = annos->clone();
            IR::Vector<IR::Expression> annoExprs;
            auto symbolId = symbols.getId(
                ControlPlaneAPI::P4RuntimeSymbolType::ACTION(), action);
            const auto* idConst =
                new IR::Constant(new IR::Type_Bits(32, false), symbolId);
            annoExprs.push_back(idConst);
            newAnnos->add(new IR::Annotation("id", annoExprs));
            action->annotations = newAnnos;
        }

        auto* paramList = new IR::ParameterList();
        for (int symbolId = 0; symbolId < action->parameters->size();
             ++symbolId) {
            auto* param = action->parameters->getParameter(symbolId)->clone();
            auto anno = ControlPlaneAPI::getIdAnnotation(param);
            if (!anno) {
                const auto* annos = param->getAnnotations();
                auto* newAnnos = annos->clone();
                IR::Vector<IR::Expression> annoExprs;
                const auto* idConst = new IR::Constant(
                    new IR::Type_Bits(32, false), symbolId + 1);
                annoExprs.push_back(idConst);
                newAnnos->add(new IR::Annotation("id", annoExprs));
                param->annotations = newAnnos;
            }
            paramList->parameters.push_back(param);
        }
        action->parameters = paramList;
        return action;
    }

 public:
    explicit MissingIdAssigner(ReferenceMap* refMap, TypeMap* typeMap,
                               const IR::ToplevelBlock* evaluatedProgram)
        : refMap(refMap), typeMap(typeMap),
          symbols(ControlPlaneAPI::P4RuntimeSymbolTable::generateSymbols(
              evaluatedProgram->getProgram(), evaluatedProgram, refMap, typeMap,
              ControlPlaneAPI::Standard::V1ModelArchHandlerBuilder()(
                  refMap, typeMap, evaluatedProgram))) {
        CHECK_NULL(typeMap);
        CHECK_NULL(refMap);
        setName("MissingIdAssigner");
    }
};

class AddMissingIdAnnotations final : public PassManager {
 public:
    AddMissingIdAnnotations(ReferenceMap* refMap, TypeMap* typeMap,
                            const IR::ToplevelBlock* toplevelBlock) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        passes.push_back(new ControlPlaneAPI::ParseP4RuntimeAnnotations());
        passes.push_back(new MissingIdAssigner(refMap, typeMap, toplevelBlock));
        // Need to refresh types after we have updated some expressions with
        // IDs.
        passes.push_back(new TypeChecking(refMap, typeMap, true));
        setName("AddMissingIdAnnotations");
    }
};

}  // namespace BMV2

}  // namespace P4

#endif /* BACKENDS_BMV2_COMMON_ADDMISSINGIDS_H_ */
