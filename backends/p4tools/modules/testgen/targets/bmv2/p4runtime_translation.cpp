#include "backends/p4tools/modules/testgen/targets/bmv2/p4runtime_translation.h"

std::vector<const IR::Annotation *>
P4Tools::P4Testgen::Bmv2::PropagateP4RuntimeTranslation::lookupP4RuntimeAnnotations(
    const P4::TypeMap &typeMap, const IR::Type *type) {
    std::vector<const IR::Annotation *> p4RuntimeAnnotations;
    const auto *typeName = type->to<IR::Type_Name>();
    if (typeName != nullptr) {
        type = typeMap.getType(typeName);
        if (type == nullptr) {
            ::error("Type %1% not found in the type map.", typeName);
            return p4RuntimeAnnotations;
        }
        type = type->getP4Type();
    }
    const auto *annotatedType = type->to<IR::IAnnotated>();
    if (annotatedType == nullptr) {
        return p4RuntimeAnnotations;
    }
    const auto *p4runtimeAnnotation = annotatedType->getAnnotation("p4runtime_translation");
    if (p4runtimeAnnotation != nullptr) {
        BUG_CHECK(!p4runtimeAnnotation->needsParsing,
                  "The @p4runtime_translation annotation should have been parsed already.");
        p4RuntimeAnnotations.push_back(p4runtimeAnnotation);
    }
    const auto *p4runtimeTranslationMappings =
        annotatedType->getAnnotation("p4runtime_translation_mappings");
    if (p4runtimeTranslationMappings != nullptr) {
        BUG_CHECK(
            !p4runtimeTranslationMappings->needsParsing,
            "The @p4runtime_translation_mappings annotation should have been parsed already.");
        p4RuntimeAnnotations.push_back(p4runtimeTranslationMappings);
    }
    return p4RuntimeAnnotations;
}

const IR::Parameter *P4Tools::P4Testgen::Bmv2::PropagateP4RuntimeTranslation::preorder(
    IR::Parameter *parameter) {
    auto p4RuntimeAnnotations = lookupP4RuntimeAnnotations(_typeMap, parameter->type);
    if (p4RuntimeAnnotations.empty()) {
        return parameter;
    }
    auto *annotationsVector = parameter->annotations->clone();
    for (const auto *p4runtimeAnnotation : p4RuntimeAnnotations) {
        annotationsVector->annotations.push_back(p4runtimeAnnotation);
    }
    parameter->annotations = annotationsVector;
    return parameter;
}

const IR::KeyElement *P4Tools::P4Testgen::Bmv2::PropagateP4RuntimeTranslation::preorder(
    IR::KeyElement *keyElement) {
    auto p4RuntimeAnnotations = lookupP4RuntimeAnnotations(_typeMap, keyElement->expression->type);
    if (p4RuntimeAnnotations.empty()) {
        return keyElement;
    }
    auto *annotationsVector = keyElement->annotations->clone();
    for (const auto *p4runtimeAnnotation : p4RuntimeAnnotations) {
        annotationsVector->annotations.push_back(p4runtimeAnnotation);
    }
    keyElement->annotations = annotationsVector;
    return keyElement;
}

P4Tools::P4Testgen::Bmv2::PropagateP4RuntimeTranslation::PropagateP4RuntimeTranslation(
    const P4::TypeMap &typeMap)
    : _typeMap(typeMap) {
    setName("PropagateP4RuntimeTranslation");
}
