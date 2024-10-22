/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wred.h"

#include "programStructure.h"

P4V1::WREDConverter::WREDConverter() { addConverter("wred"_cs, this); }

const IR::Type_Extern *P4V1::WREDConverter::convertExternType(P4V1::ProgramStructure *structure,
                                                              const IR::Type_Extern *, cstring) {
    if (use_v1model()) structure->include("tofino/wred.p4"_cs);
    return nullptr;
}

const IR::Declaration_Instance *P4V1::WREDConverter::convertExternInstance(
    P4V1::ProgramStructure *structure, const IR::Declaration_Instance *ext, cstring name,
    IR::IndexedVector<IR::Declaration> *) {
    auto *et = ext->type->to<IR::Type_Extern>();
    BUG_CHECK(et && et->name == "wred", "Extern %s is not wred type, but %s", ext, ext->type);
    ExpressionConverter conv(structure);
    const IR::Type *input_type = nullptr;
    const IR::Expression *instance_count = nullptr;
    const IR::Expression *drop_value = nullptr;
    const IR::Expression *no_drop_value = nullptr;
    const IR::Expression *table = nullptr;
    bool direct = false;
    for (auto prop : Values(ext->properties)) {
        const IR::Expression *val = nullptr;
        if (auto ev = prop->value->to<IR::ExpressionValue>()) val = conv.convert(ev->expression);
        if (prop->name == "wred_input") {
            input_type = val ? val->type : nullptr;
        } else if (prop->name == "direct" || prop->name == "static") {
            if (table) error("wred %s specifies both 'direct' and 'static'", ext);
            direct = prop->name == "direct";
            table = val;
        } else if (prop->name == "instance_count") {
            instance_count = val;
        } else if (prop->name == "drop_value") {
            drop_value = val;
        } else if (prop->name == "no_drop_value") {
            no_drop_value = val;
        } else {
            error("Unknown property %s on wred", prop);
            continue;
        }
        if (!val) error("%s: %s property is not an expression", prop->name, prop->value->srcInfo);
    }
    if (!input_type) {
        error("No wred_input in wred %s", ext);
        input_type = IR::Type::Bits::get(8); /* random type to avoid nullptr deref */
    }
    if (direct && instance_count)
        error("wred %s specifies both 'direct' and 'instance_count'", ext);
    if (!drop_value) drop_value = new IR::Constant(IR::Type::Bits::get(8), (1U << 8) - 1);
    if (!no_drop_value) no_drop_value = new IR::Constant(IR::Type::Bits::get(8), 0);

    auto args = new IR::Vector<IR::Argument>;
    if (instance_count) args->push_back(new IR::Argument(instance_count));
    args->push_back(new IR::Argument(drop_value));
    args->push_back(new IR::Argument(no_drop_value));

    auto *externalName = new IR::StringLiteral(IR::ID("." + name));
    auto *annotations = new IR::Annotations({new IR::Annotation(IR::ID("name"), {externalName})});

    if (instance_count) {
        auto type = new IR::Type_Specialized(
            new IR::Type_Name("Wred"),
            new IR::Vector<IR::Type>({input_type, IR::Type::Bits::get(32)}));
        return new IR::Declaration_Instance(ext->srcInfo, name, annotations, type, args);
    } else {
        auto type = new IR::Type_Specialized(new IR::Type_Name("DirectWred"),
                                             new IR::Vector<IR::Type>({input_type}));
        return new IR::Declaration_Instance(ext->srcInfo, name, annotations, type, args);
    }
}

const IR::Statement *P4V1::WREDConverter::convertExternCall(P4V1::ProgramStructure *structure,
                                                            const IR::Declaration_Instance *ext,
                                                            const IR::Primitive *prim) {
    auto *et = ext->type->to<IR::Type_Extern>();
    BUG_CHECK(et && et->name == "wred", "Extern %s is not wred type, but %s", ext, ext->type);
    ExpressionConverter conv(structure);
    const IR::Statement *rv = nullptr;
    auto args = new IR::Vector<IR::Argument>();
    BUG_CHECK(prim->operands.size() >= 2 && prim->operands.size() <= 3,
              "Expected 2 or 3 operands for %s", prim->name);
    if (auto prop = ext->properties.get<IR::Property>("wred_input"_cs)) {
        if (auto ev = prop->value->to<IR::ExpressionValue>()) {
            args->push_back(new IR::Argument(conv.convert(ev->expression)));
        } else {
            error("%s: wred_input property is not an expression", prop->value->srcInfo);
            return nullptr;
        }
    } else {
        error("No wred_input in %s", ext);
    }
    IR::BlockStatement *block = nullptr;
    if (prim->name == "execute_from_hash") {
        cstring temp = structure->makeUniqueName("temp"_cs);
        block = P4V1::generate_hash_block_statement(structure, prim, temp, conv, 3);
        args->push_back(new IR::Argument(
            new IR::Cast(IR::Type_Bits::get(32), new IR::PathExpression(new IR::Path(temp)))));
    } else if (prim->name == "execute") {
        if (prim->operands.size() == 3)
            args->push_back(new IR::Argument(conv.convert(prim->operands.at(2))));
    } else {
        BUG("Unknown method %s in wred", prim->name);
    }
    auto extref = new IR::PathExpression(structure->externs.get(ext));
    auto method = new IR::Member(prim->srcInfo, extref, "execute"_cs);
    auto mc = new IR::MethodCallExpression(prim->srcInfo, IR::Type::Bits::get(8), method, args);
    auto dest = conv.convert(prim->operands.at(1));
    rv = structure->assign(prim->srcInfo, dest, mc, dest->type);
    if (block) {
        block->push_back(rv);
        rv = block;
    }
    return rv;
}

P4V1::WREDConverter P4V1::WREDConverter::singleton;
