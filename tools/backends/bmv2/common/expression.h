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

#ifndef BACKENDS_BMV2_COMMON_EXPRESSION_H_
#define BACKENDS_BMV2_COMMON_EXPRESSION_H_

#include "ir/ir.h"
#include "lower.h"
#include "lib/gmputil.h"
#include "lib/json.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "programStructure.h"

namespace BMV2 {

/**
   Inserts casts and narrowing operations to implement correctly the semantics of
   P4-16 arithmetic on top of unbounded precision arithmetic.  For example,
   in P4-16 adding two 32-bit values should produce a 32-bit value, but using
   unbounded arithmetic, as in BMv2, it could produce a 33-bit value.
 */
class ArithmeticFixup : public Transform {
    P4::TypeMap* typeMap;
 public:
    const IR::Expression* fix(const IR::Expression* expr, const IR::Type_Bits* type);
    const IR::Node* updateType(const IR::Expression* expression);
    const IR::Node* postorder(IR::Expression* expression) override;
    const IR::Node* postorder(IR::Operation_Binary* expression) override;
    const IR::Node* postorder(IR::Neg* expression) override;
    const IR::Node* postorder(IR::Cmpl* expression) override;
    const IR::Node* postorder(IR::Cast* expression) override;
    explicit ArithmeticFixup(P4::TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); }
};

class ExpressionConverter : public Inspector {
    P4::ReferenceMap*    refMap;
    P4::TypeMap*         typeMap;
    ProgramStructure*    structure;
    P4::P4CoreLibrary&   corelib;
    cstring              scalarsName;

    /// after translating an Expression to JSON, save the result to 'map'.
    std::map<const IR::Expression*, Util::IJson*> map;
    bool leftValue;  // true if converting a left value
    // in some cases the bmv2 JSON requires a 'bitwidth' attribute for hex
    // strings (e.g. for constants in calculation inputs). When this flag is set
    // to true, we add this attribute.
    bool withConstantWidths{false};

 public:
    ExpressionConverter(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                        ProgramStructure* structure, cstring scalarsName) :
            refMap(refMap), typeMap(typeMap), structure(structure),
            corelib(P4::P4CoreLibrary::instance),
            scalarsName(scalarsName), leftValue(false), simpleExpressionsOnly(false) {}
    /// If this is 'true' we fail to convert complex expressions.
    /// This is used for table key expressions, for example.
    bool simpleExpressionsOnly;

    /// Non-null if the expression refers to a parameter from the enclosing control
    const IR::Parameter* enclosingParamReference(const IR::Expression* expression);

    // Each architecture typically has some special parameters that requires
    // special handling. The examples are standard_metadata in the v1model and
    // packet path related metadata in PSA. Each target should subclass the
    // ExpressionConverter and implement this function with target-specific
    // handling code to deal with the special parameters.
    virtual Util::IJson* convertParam(const IR::Parameter* param, cstring fieldName) = 0;

    Util::IJson* get(const IR::Expression* expression) const;
    Util::IJson* fixLocal(Util::IJson* json);

    /**
     * Convert an expression into JSON
     * @param e   expression to convert
     * @param doFixup  Insert masking operations for operands to ensure that the result
     *                 matches the specification.  BMv2 does arithmetic using unbounded
     *                 precision, but the spec requires fixed precision, specified by the types.
     * @param wrap     Wrap the result into an additiona JSON expression block.
     *                 See the BMv2 JSON spec.
     * @param convertBool  Wrap the result into a cast from boolean to data (b2d JSON).
     */
    Util::IJson* convert(const IR::Expression* e, bool doFixup = true,
                         bool wrap = true, bool convertBool = false);
    Util::IJson* convertLeftValue(const IR::Expression* e);
    Util::IJson* convertWithConstantWidths(const IR::Expression* e);

    void postorder(const IR::BoolLiteral* expression) override;
    void postorder(const IR::MethodCallExpression* expression) override;
    void postorder(const IR::Cast* expression) override;
    void postorder(const IR::Slice* expression) override;
    void postorder(const IR::AddSat* expression) override { saturated_binary(expression); }
    void postorder(const IR::SubSat* expression) override { saturated_binary(expression); }
    void postorder(const IR::Constant* expression) override;
    void postorder(const IR::ArrayIndex* expression) override;
    void postorder(const IR::Member* expression) override;
    void postorder(const IR::Mux* expression) override;
    void postorder(const IR::IntMod* expression) override;
    void postorder(const IR::Operation_Binary* expression) override;
    void postorder(const IR::ListExpression* expression) override;
    void postorder(const IR::StructInitializerExpression* expression) override;
    void postorder(const IR::Operation_Unary* expression) override;
    void postorder(const IR::PathExpression* expression) override;
    void postorder(const IR::TypeNameExpression* expression) override;
    void postorder(const IR::Expression* expression) override;
    void mapExpression(const IR::Expression* expression, Util::IJson* json);

 private:
    void binary(const IR::Operation_Binary* expression);
    void saturated_binary(const IR::Operation_Binary* expression);
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_EXPRESSION_H_ */
