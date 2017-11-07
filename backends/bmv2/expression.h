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

#ifndef _BACKENDS_BMV2_EXPRESSION_H_
#define _BACKENDS_BMV2_EXPRESSION_H_

#include "ir/ir.h"
#include "lower.h"
#include "lib/gmputil.h"
#include "lib/json.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "helpers.h"
#include "backend.h"

namespace BMV2 {

class Backend;

class ArithmeticFixup : public Transform {
    P4::TypeMap* typeMap;
 public:
    const IR::Expression* fix(const IR::Expression* expr, const IR::Type_Bits* type);
    const IR::Node* updateType(const IR::Expression* expression);
    const IR::Node* postorder(IR::Expression* expression) override;
    const IR::Node* postorder(IR::Operation_Binary* expression) override;
    const IR::Node* postorder(IR::Neg* expression) override;
    const IR::Node* postorder(IR::Cast* expression) override;
    explicit ArithmeticFixup(P4::TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); }
};

class ExpressionConverter : public Inspector {
    Backend*           backend;
    P4::P4CoreLibrary& corelib;
    cstring            scalarsName;

    /// after translating an Expression to JSON, save the result to 'map'.
    std::map<const IR::Expression*, Util::IJson*> map;
    bool leftValue;  // true if converting a left value
    // in some cases the bmv2 JSON requires a 'bitwidth' attribute for hex
    // strings (e.g. for constants in calculation inputs). When this flag is set
    // to true, we add this attribute.
    bool withConstantWidths{false};

 public:
    ExpressionConverter(Backend *backend, cstring scalarsName) :
            backend(backend), corelib(P4::P4CoreLibrary::instance),
            scalarsName(scalarsName), leftValue(false), simpleExpressionsOnly(false) {}
    bool simpleExpressionsOnly;  // if set we fail to convert complex expressions

    // Non-null if the expression refers to a parameter from the enclosing control
    const IR::Parameter* enclosingParamReference(const IR::Expression* expression);
    void binary(const IR::Operation_Binary* expression);
    Util::IJson* get(const IR::Expression* expression) const;
    Util::IJson* fixLocal(Util::IJson* json);

    // doFixup = true -> insert masking operations for proper arithmetic implementation
    Util::IJson* convert(const IR::Expression* e, bool doFixup = true,
                         bool wrap = true, bool convertBool = false);
    Util::IJson* convertLeftValue(const IR::Expression* e);
    Util::IJson* convertWithConstantWidths(const IR::Expression* e);

    void postorder(const IR::BoolLiteral* expression) override;
    void postorder(const IR::MethodCallExpression* expression) override;
    void postorder(const IR::Cast* expression) override;
    void postorder(const IR::Constant* expression) override;
    void postorder(const IR::ArrayIndex* expression) override;
    void postorder(const IR::Member* expression) override;
    void postorder(const IR::Mux* expression) override;
    void postorder(const IR::IntMod* expression) override;
    void postorder(const IR::Operation_Binary* expression) override;
    void postorder(const IR::ListExpression* expression) override;
    void postorder(const IR::Operation_Unary* expression) override;
    void postorder(const IR::PathExpression* expression) override;
    void postorder(const IR::TypeNameExpression* expression) override;
    void postorder(const IR::Expression* expression) override;
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_EXPRESSION_H_ */
