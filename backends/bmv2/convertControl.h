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

#ifndef _BACKENDS_BMV2_CONVERTCONTROL_H_
#define _BACKENDS_BMV2_CONVERTCONTROL_H_

#include "ir/ir.h"
#include "lib/json.h"
#include "analyzer.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "midend/convertEnums.h"
#include "expressionConverter.h"
#include "helpers.h"

namespace BMV2 {

// This pass makes sure that when several match tables share a selector, they use the same input for
// the selection algorithm. This is because bmv2 considers that the selection key is part of the
// action_selector while v1model.p4 considers that it belongs to the table match key definition.
class SharedActionSelectorCheck : public Inspector {
    P4::ReferenceMap* refMap;
    P4::TypeMap*      typeMap;
    using Input = std::vector<const IR::Expression *>;
    std::map<const IR::Declaration_Instance *, Input> selector_input_map{};

  static bool checkSameKeyExpr(const IR::Expression* expr0, const IR::Expression* expr1) {
      if (expr0->node_type_name() != expr1->node_type_name())
          return false;
      if (auto pe0 = expr0->to<IR::PathExpression>()) {
          auto pe1 = expr1->to<IR::PathExpression>();
          return pe0->path->name == pe1->path->name &&
              pe0->path->absolute == pe1->path->absolute;
      } else if (auto mem0 = expr0->to<IR::Member>()) {
          auto mem1 = expr1->to<IR::Member>();
          return checkSameKeyExpr(mem0->expr, mem1->expr) && mem0->member == mem1->member;
      } else if (auto l0 = expr0->to<IR::Literal>()) {
          auto l1 = expr1->to<IR::Literal>();
          return *l0 == *l1;
      } else if (auto ai0 = expr0->to<IR::ArrayIndex>()) {
          auto ai1 = expr1->to<IR::ArrayIndex>();
          return checkSameKeyExpr(ai0->left, ai1->left) && checkSameKeyExpr(ai0->right, ai1->right);
      }
      return false;
  }

 public:
    explicit SharedActionSelectorCheck(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
        refMap(refMap), typeMap(typeMap) {}

    const Input &get_selector_input(const IR::Declaration_Instance* selector) const {
        return selector_input_map.at(selector);
    }

    bool preorder(const IR::P4Table* table) override {
        auto implementation = table->properties->getProperty("implementation");
                //v2model.tableAttributes.tableImplementation.name);
        if (implementation == nullptr) return false;
        if (!implementation->value->is<IR::ExpressionValue>()) {
          ::error("%1%: expected expression for property", implementation);
          return false;
        }
        auto propv = implementation->value->to<IR::ExpressionValue>();
        if (!propv->expression->is<IR::PathExpression>()) return false;
        auto pathe = propv->expression->to<IR::PathExpression>();
        auto decl = refMap->getDeclaration(pathe->path, true);
        if (!decl->is<IR::Declaration_Instance>()) {
            ::error("%1%: expected a reference to an instance", pathe);
            return false;
        }
        auto dcltype = typeMap->getType(pathe, true);
        if (!dcltype->is<IR::Type_Extern>()) {
            ::error("%1%: unexpected type for implementation", dcltype);
            return false;
        }
        auto type_extern_name = dcltype->to<IR::Type_Extern>()->name;
        //if (type_extern_name != v2model.action_selector.name) return false;

        auto key = table->getKey();
        Input input;
        for (auto ke : key->keyElements) {
            auto mt = refMap->getDeclaration(ke->matchType->path, true)->to<IR::Declaration_ID>();
            BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
            //if (mt->name.name != v2model.selectorMatchType.name) continue;
            input.push_back(ke->expression);
        }
        auto decl_instance = decl->to<IR::Declaration_Instance>();
        auto it = selector_input_map.find(decl_instance);
        if (it == selector_input_map.end()) {
            selector_input_map[decl_instance] = input;
            return false;
        }
        // returns true if inputs are the same, false otherwise
        auto cmp_inputs = [](const Input &i1, const Input &i2) {
            for (auto e1 : i1) {
                auto cmp_e = [e1](const IR::Expression *e2) {
                    return checkSameKeyExpr(e1, e2);
                };
                if (std::find_if(i2.begin(), i2.end(), cmp_e) == i2.end()) return false;
            }
            return true;
        };

        if (!cmp_inputs(it->second, input)) {
            ::error(
                 "Action selector '%1%' is used by multiple tables with different selector inputs",
                 decl);
        }

        return false;
    }
};

class DoControlBlockConversion : public Inspector {
    P4::ReferenceMap*    refMap;
    P4::TypeMap*         typeMap;
    ExpressionConverter* conv;
    ProgramParts*        structure;
    Util::JsonArray*     counters;
    Util::JsonArray*     pipelines;
    P4::P4CoreLibrary&   corelib;
    P4::V2Model&         v2model;

 protected:
    // helper function that create a JSON object for extern block
    Util::IJson* createExternInstance(cstring name, cstring type);
    Util::IJson* convertTable(const CFG::TableNode* node,
                              Util::JsonArray* counters,
                              Util::JsonArray* action_profiles);
    void convertTableEntries(const IR::P4Table *table, Util::JsonObject *jsonTable);
    cstring getKeyMatchType(const IR::KeyElement *ke);
    // Return 'true' if the table is 'simple'
    bool handleTableImplementation(const IR::Property* implementation, const IR::Key* key,
                                   Util::JsonObject* table, Util::JsonArray* action_profiles);
    Util::IJson* convertIf(const CFG::IfNode* node, cstring prefix);
    Util::IJson* convertControl(const IR::ControlBlock* block, cstring name,
                                Util::JsonArray *counters, Util::JsonArray* meters,
                                Util::JsonArray* registers);
 public:
    bool preorder(const IR::PackageBlock* b) override;
    bool preorder(const IR::ControlBlock* b) override;
    bool preorder(const IR::Declaration_Instance* d) override;

    explicit DoControlBlockConversion(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                                      ExpressionConverter* conv,
                                      ProgramParts* structure, Util::JsonArray* pipelines,
                                      Util::JsonArray* counters) :
        refMap(refMap), typeMap(typeMap), conv(conv),
        structure(structure), counters(counters), pipelines(pipelines),
        corelib(P4::P4CoreLibrary::instance),
        v2model(P4::V2Model::instance)
        { CHECK_NULL(conv); }

};

class ConvertControl final : public PassManager {
 public:
    ConvertControl(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                   ExpressionConverter* conv,
                   ProgramParts* structure, Util::JsonArray* pipelines,
                   Util::JsonArray* counters) {
        passes.push_back(new DoControlBlockConversion(refMap, typeMap, conv, structure, pipelines, counters));
        setName("ConvertControl");
    }
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_CONVERTCONTROL_H_ */
