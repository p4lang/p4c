/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "lpf.h"
#include "programStructure.h"

P4V1::LpfConverter::LpfConverter() {
    addConverter("lpf"_cs, this);
}

const IR::Type_Extern *P4V1::LpfConverter::convertExternType(P4V1::ProgramStructure *structure,
                                                             const IR::Type_Extern *, cstring) {
    if (use_v1model())
        structure->include("tofino/lpf.p4"_cs);
    return nullptr;
}

const IR::Declaration_Instance *P4V1::LpfConverter::convertExternInstance(
        P4V1::ProgramStructure *structure, const IR::Declaration_Instance *ext, cstring name,
        IR::IndexedVector<IR::Declaration> *) {
    auto *et = ext->type->to<IR::Type_Extern>();
    BUG_CHECK(et && et->name == "lpf", "Extern %s is not lpf type, but %s", ext, ext->type);
    ExpressionConverter conv(structure);
    const IR::Type *filt_type = nullptr;
    const IR::Expression *instance_count = nullptr;
    const IR::Expression *table = nullptr;
    bool direct = false;
    for (auto prop : Values(ext->properties)) {
        const IR::Expression *val = nullptr;
        if (auto ev = prop->value->to<IR::ExpressionValue>())
            val = conv.convert(ev->expression);
        if (prop->name == "filter_input") {
            filt_type = val ? val->type : nullptr;
        } else if (prop->name == "direct" || prop->name == "static") {
            if (table) error("lpf %s specifies both 'direct' and 'static'", ext);
            direct = prop->name == "direct";
            table = val;
        } else if (prop->name == "instance_count") {
            instance_count = val;
        } else {
            error("Unknown property %s on lpf", prop);
            continue; }
        if (!val)
            error("%s: %s property is not an expression", prop->name, prop->value->srcInfo); }
    if (direct && instance_count)
        error("lpf %s specifies both 'direct' and 'instance_count'", ext);

    auto* externalName = new IR::StringLiteral(IR::ID("." + name));
    auto* annotations = new IR::Annotations({
        new IR::Annotation(IR::ID("name"), { externalName })});
    auto args = new IR::Vector<IR::Argument>();
    if (instance_count) {
        args->push_back(new IR::Argument(instance_count));
        auto type = new IR::Type_Specialized(
            new IR::Type_Name("Lpf"),
            new IR::Vector<IR::Type>({ filt_type, IR::Type::Bits::get(32) }));
        return new IR::Declaration_Instance(ext->srcInfo, name, annotations, type, args);
    } else {
        auto type = new IR::Type_Specialized(
            new IR::Type_Name("DirectLpf"),
            new IR::Vector<IR::Type>({ filt_type }));
        return new IR::Declaration_Instance(ext->srcInfo, name, annotations, type, args);
    }
}

const IR::Statement *P4V1::LpfConverter::convertExternCall(P4V1::ProgramStructure *structure,
        const IR::Declaration_Instance *ext, const IR::Primitive *prim) {
    auto *et = ext->type->to<IR::Type_Extern>();
    BUG_CHECK(et && et->name == "lpf", "Extern %s is not lpf type, but %s", ext, ext->type);
    ExpressionConverter conv(structure);
    const IR::Statement *rv = nullptr;
    auto args = new IR::Vector<IR::Argument>();
    BUG_CHECK(prim->operands.size() >= 2 && prim->operands.size() <= 3,
              "Expected 2 or 3 operands for %s", prim->name);
    const IR::Type *filt_type = nullptr;
    if (auto prop = ext->properties.get<IR::Property>("filter_input"_cs)) {
        if (auto ev = prop->value->to<IR::ExpressionValue>()) {
            filt_type = ev->expression->type;
            args->push_back(new IR::Argument(conv.convert(ev->expression)));
        } else {
            error("%s: filter_input property is not an expression", prop->value->srcInfo);
            return nullptr; }
    } else {
        error("No filter_input in %s", ext); }
    IR::BlockStatement *block = nullptr;
    if (prim->name == "execute_from_hash") {
        cstring temp = structure->makeUniqueName("temp"_cs);
        block = P4V1::generate_hash_block_statement(structure, prim, temp, conv, 3);
        args->push_back(new IR::Argument(new IR::Cast(IR::Type_Bits::get(32),
                        new IR::PathExpression(new IR::Path(temp)))));
    } else if (prim->name == "execute") {
        if (prim->operands.size() == 3)
            args->push_back(new IR::Argument(conv.convert(prim->operands.at(2))));
    } else {
        BUG("Unknown method %s in lpf", prim->name);
    }
    auto extref = new IR::PathExpression(structure->externs.get(ext));
    auto method = new IR::Member(prim->srcInfo, extref, "execute"_cs);
    auto mc = new IR::MethodCallExpression(prim->srcInfo, filt_type, method, args);
    auto dest = conv.convert(prim->operands.at(1));
    rv = structure->assign(prim->srcInfo, dest, mc, dest->type);
    if (block) {
        block->push_back(rv);
        rv = block; }
    return rv;
}

P4V1::LpfConverter P4V1::LpfConverter::singleton;
