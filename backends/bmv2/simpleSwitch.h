/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _BACKENDS_BMV2_SIMPLESWITCH_H_
#define _BACKENDS_BMV2_SIMPLESWITCH_H_

#include <algorithm>
#include <cstring>
#include "frontends/p4/fromv1.0/v1model.h"
#include "sharedActionSelectorCheck.h"

namespace BMV2 {

class Backend;

}  // namespace BMV2

namespace P4V1 {

// Unfortunately, we cannot use ExpressionConverter as the subclass name.
// Dynamic dispatching does not seems to work in clang, when the subclass has
// the same name as the superclass, and they are in different namespaces.
class SimpleSwitchExpressionConverter : public BMV2::ExpressionConverter {
    const IR::ToplevelBlock* toplevel;

 public:
    SimpleSwitchExpressionConverter(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
        BMV2::ProgramParts* structure, cstring scalarsName, const IR::ToplevelBlock* toplevel) :
        BMV2::ExpressionConverter(refMap, typeMap, structure, scalarsName), toplevel(toplevel) { }

    void modelError(const char* format, const IR::Node* node) {
        ::error(format, node);
        ::error("Are you using an up-to-date v1model.p4?");
    }

    const IR::P4Control* getIngress(const IR::ToplevelBlock* blk) {
        auto main = blk->getMain();
        auto ctrl = main->findParameterValue("ig");
        if (ctrl == nullptr)
            return nullptr;
        if (!ctrl->is<IR::ControlBlock>()) {
            modelError("%1%: main package  match the expected model", main);
            return nullptr;
        }
        return ctrl->to<IR::ControlBlock>()->container;
    }

    const IR::P4Control* getEgress(const IR::ToplevelBlock* blk) {
        auto main = blk->getMain();
        auto ctrl = main->findParameterValue("eg");
        if (ctrl == nullptr)
            return nullptr;
        if (!ctrl->is<IR::ControlBlock>()) {
            modelError("%1%: main package  match the expected model", main);
            return nullptr;
        }
        return ctrl->to<IR::ControlBlock>()->container;
    }

    const IR::P4Parser* getParser(const IR::ToplevelBlock* blk) {
        auto main = blk->getMain();
        auto ctrl = main->findParameterValue("p");
        if (ctrl == nullptr)
            return nullptr;
        if (!ctrl->is<IR::ParserBlock>()) {
            modelError("%1%: main package  match the expected model", main);
            return nullptr;
        }
        return ctrl->to<IR::ParserBlock>()->container;
    }

    bool isStandardMetadataParameter(const IR::Parameter* param) {
        auto parser = getParser(toplevel);
        auto params = parser->getApplyParameters();
        if (params->size() != 4) {
            modelError("%1%: Expected 4 parameter for parser", parser);
            return false;
        }
        if (params->parameters.at(3) == param)
            return true;

        auto ingress = getIngress(toplevel);
        params = ingress->getApplyParameters();
        if (params->size() != 3) {
            modelError("%1%: Expected 3 parameter for ingress", ingress);
            return false;
        }
        if (params->parameters.at(2) == param)
            return true;

        auto egress = getEgress(toplevel);
        params = egress->getApplyParameters();
        if (params->size() != 3) {
            modelError("%1%: Expected 3 parameter for egress", egress);
            return false;
        }
        if (params->parameters.at(2) == param)
            return true;

        return false;
    }

    Util::IJson* convertParam(const IR::Parameter* param, cstring fieldName) override {
        if (isStandardMetadataParameter(param)) {
            auto result = new Util::JsonObject();
            result->emplace("type", "field");
            auto e = BMV2::mkArrayField(result, "value");
            e->append(BMV2::V1ModelProperties::jsonMetadataParameterName);
            e->append(fieldName);
            return result;
        }
        return nullptr;
    }
};

class SimpleSwitch {
    BMV2::Backend* backend;
    V1Model&       v1model;

 protected:
    void addToFieldList(const IR::Expression* expr, Util::JsonArray* fl);
    int createFieldList(const IR::Expression* expr, cstring group,
                        cstring listName, Util::JsonArray* field_lists);
    cstring convertHashAlgorithm(cstring algorithm);
    cstring createCalculation(cstring algo, const IR::Expression* fields,
                              Util::JsonArray* calculations, bool usePayload, const IR::Node* node);

 public:
    void modelError(const char* format, const IR::Node* place) const;
    void convertExternObjects(Util::JsonArray *result, const P4::ExternMethod *em,
                              const IR::MethodCallExpression *mc, const IR::StatOrDecl *s,
                              const bool& emitExterns);
    void convertExternFunctions(Util::JsonArray *result, const P4::ExternFunction *ef,
                                const IR::MethodCallExpression *mc, const IR::StatOrDecl* s);
    void convertExternInstances(const IR::Declaration *c,
                                const IR::ExternBlock* eb, Util::JsonArray* action_profiles,
                                BMV2::SharedActionSelectorCheck& selector_check,
                                const bool& emitExterns);
    void convertChecksum(const IR::BlockStatement* body, Util::JsonArray* checksums,
                         Util::JsonArray* calculations, bool verify);
    void convertActionBody(const IR::Vector<IR::StatOrDecl>* body,
                           Util::JsonArray* result);
    void convertActionParams(const IR::ParameterList *parameters, Util::JsonArray* params);
    void createActions();

    void setPipelineControls(const IR::ToplevelBlock* blk, std::set<cstring>* controls,
                             std::map<cstring, cstring>* map);
    void setNonPipelineControls(const IR::ToplevelBlock* blk, std::set<cstring>* controls);
    void setComputeChecksumControls(const IR::ToplevelBlock* blk, std::set<cstring>* controls);
    void setVerifyChecksumControls(const IR::ToplevelBlock* blk, std::set<cstring>* controls);
    void setDeparserControls(const IR::ToplevelBlock* blk, std::set<cstring>* controls);

    const IR::P4Parser*  getParser(const IR::ToplevelBlock* blk);
    const IR::P4Control* getCompute(const IR::ToplevelBlock* blk);
    const IR::P4Control* getVerify(const IR::ToplevelBlock* blk);

    explicit SimpleSwitch(BMV2::Backend* backend) :
        backend(backend), v1model(V1Model::instance)
    { CHECK_NULL(backend); }
};

}  // namespace P4V1

#endif /* _BACKENDS_BMV2_SIMPLESWITCH_H_ */
