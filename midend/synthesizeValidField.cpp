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

#include "synthesizeValidField.h"

#include <boost/range/combine.hpp>
#include <boost/range/irange.hpp>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

/// Adds a valid bit to all header types.
class AddValidField final : public Modifier {
 public:
    AddValidField() { setName("AddValidField"); }
 private:
    bool preorder(IR::Type_Header* header) override {
        if (header->getField("$valid$") != nullptr) return true;
        auto annotations = new IR::Annotations(
            {new IR::Annotation(IR::Annotation::hiddenAnnotation, {})});
        auto field = new IR::StructField("$valid$", annotations, IR::Type::Bits::get(1));
        header->fields.push_back(field);
        LOG2("Added field to " << header);
        return true;
    }
};

/// Replaces calls to isValid() with references to the header's valid bit.
class RewriteIsValidCalls final : public Transform {
 public:
    RewriteIsValidCalls(P4::ReferenceMap* refMap, P4::TypeMap* typeMap)
            : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("RewriteIsValidCalls");
    }

 private:
    const IR::Node* postorder(IR::MethodCallExpression* expr) override;

    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
};

const IR::Node*
RewriteIsValidCalls::postorder(IR::MethodCallExpression* expr) {
    auto instance = P4::MethodInstance::resolve(expr, refMap, typeMap);
    if (!instance->is<P4::BuiltInMethod>()) return expr;
    auto builtin = instance->to<P4::BuiltInMethod>();
    if (builtin->name != IR::Type_Header::isValid) return expr;

    // The isValid() builtin is defined for both headers and header unions, but
    // only headers have a valid bit - header unions work differently
    // internally - so we'll leave isValid() invocations on header unions as-is.
    auto type = typeMap->getType(builtin->appliedTo, true);
    if (type->is<IR::Type_HeaderUnion>()) return expr;
    BUG_CHECK(type->is<IR::Type_Header>(),
              "Invoking isValid() on unexpected type %1%", type);

    // If isValid() is being used as a table key element, it already behaves
    // like a bit<1> value; just replace it with a reference to the valid bit.
    auto member = new IR::Member(expr->srcInfo, builtin->appliedTo, "$valid$");
    if (getParent<IR::KeyElement>() != nullptr) return member;

    // In other contexts, rewrite isValid() into a comparison with a constant.
    // This maintains a boolean type for the overall expression.
    auto constant = new IR::Constant(IR::Type::Bits::get(1), 1);
    LOG2("Rewrote " << expr);
    return new IR::Equ(expr->srcInfo, member, constant);
}

class RewriteIsValidTableEntries final : public Transform {
 public:
    RewriteIsValidTableEntries() { setName("RewriteIsValidTableEntries"); }

 private:
    const IR::Node* preorder(IR::P4Table* table) override {
        using namespace boost;

        // If this table has no key or no const entries, there's nothing to do.
        auto key = table->getKey();
        if (key == nullptr) return table;
        if (table->getEntries() == nullptr) return table;

        // Find the indices of key elements which are simple references to valid
        // bits. These were produced by RewriteIsValidCalls; originally they
        // were calls to isValid(). We need to rewrite the corresponding const
        // table entry keys, since the type of these key elements has changed
        // from boolean to bit<1>. Note that any isValid() calls which were
        // nested in another expression were transformed in a way that preserved
        // their boolean type, so we don't need to do anything special for them.
        auto& elems = key->keyElements;
        std::vector<unsigned> indicesToRewrite;
        for (auto elem : combine(elems, irange(size_t(0), elems.size()))) {
            auto expression = elem.get<0>()->expression;
            if (!expression->is<IR::Member>()) continue;
            if (expression->to<IR::Member>()->member != "$valid$") continue;
            indicesToRewrite.push_back(elem.get<1>());
        }
        if (indicesToRewrite.empty()) return table;

        // Rewrite the const table entry keys. A 'true' or 'false' literal that
        // matched an isValid() key element becomes a constant '1' or '0'.
        auto* oldPs = table->properties;
        auto newPs = transformAllMatching<IR::Entry>(oldPs, [&](IR::Entry* e) {
            auto newKey = e->keys->clone();
            for (auto index : indicesToRewrite) {
                auto value = newKey->components[index];
                BUG_CHECK(value->is<IR::BoolLiteral>(),
                          "Expected boolean literal: %1%", value);
                unsigned asBit = value->to<IR::BoolLiteral>()->value ? 1 : 0;
                newKey->components[index] =
                    new IR::Constant(IR::Type::Bits::get(1), asBit);
            }
            LOG2("Rewrote " << e);
            e->keys = newKey;
            return e;
        });

        table->properties = newPs->to<IR::TableProperties>();
        return table;
    }
};

SynthesizeValidField::SynthesizeValidField(P4::ReferenceMap* refMap,
                                           P4::TypeMap* typeMap) {
    setName("SynthesizeValidField");
    passes.push_back(new AddValidField);
    passes.push_back(new P4::ClearTypeMap(typeMap));  // Type definitions changed!
    passes.push_back(new P4::TypeChecking(refMap, typeMap));
    passes.push_back(new RewriteIsValidCalls(refMap, typeMap));
    passes.push_back(new RewriteIsValidTableEntries);
}

}  // namespace P4
