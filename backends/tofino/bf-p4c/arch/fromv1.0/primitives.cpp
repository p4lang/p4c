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

#include <cmath>

#include "bf-p4c/arch/fromv1.0/programStructure.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/device.h"
#include "frontends/p4-14/fromv1.0/converters.h"

namespace P4 {
namespace P4V1 {

#define OPS_CK(primitive, n) \
    BUG_CHECK((primitive)->operands.size() == n, "Expected " #n " operands for %1%", primitive)

// Concatenate a field string with list of fields within a method call
// argument. The field list is specified as a ListExpression
static void generate_fields_string(const IR::Expression *expr, std::string &fieldString) {
    std::ostringstream fieldListString;
    if (auto fieldList = expr->to<IR::ListExpression>()) {
        for (auto comp : fieldList->components) {
            fieldListString << comp;
        }
    }
    fieldString += fieldListString.str();
}

// Generate a 64-bit hash for input field string, lookup hash vector for
// presence of hash, add new entry if not found. This function is common to
// resubmit, digest and clone indexing
static unsigned getIndex(const IR::Expression *expr,
                         std::map<unsigned long, unsigned> &hashIndexMap) {
    std::string fieldsString;
    generate_fields_string(expr, fieldsString);
    std::hash<std::string> hashFn;

    auto fieldsStringHash = hashFn(fieldsString);
    if (hashIndexMap.count(fieldsStringHash) == 0)
        hashIndexMap.emplace(fieldsStringHash, hashIndexMap.size());

    return hashIndexMap[fieldsStringHash];
}

CONVERT_PRIMITIVE(bypass_egress) {
    if (primitive->operands.size() != 0) return nullptr;
    if (use_v1model()) structure->include("tofino/p4_14_prim.p4"_cs, "-D_TRANSLATE_TO_V1MODEL"_cs);
    return new IR::MethodCallStatement(primitive->srcInfo,
                                       IR::ID(primitive->srcInfo, "bypass_egress"_cs), {});
}

static cstring makeHashCall(ProgramStructure *structure, IR::BlockStatement *block,
                            const IR::Expression *field_list) {
    ExpressionConverter conv(structure);
    auto flc = structure->getFieldListCalculation(field_list);
    if (flc == nullptr) {
        error("%1%: Expected a field_list_calculation", field_list);
        return cstring();
    }
    auto ttype = IR::Type_Bits::get(flc->output_width);
    cstring temp = structure->makeUniqueName("temp"_cs);
    block->push_back(new IR::Declaration_Variable(temp, ttype));

    auto fl = structure->getFieldLists(flc);
    if (fl == nullptr) return nullptr;
    const IR::ListExpression *listExp = conv.convert(fl)->to<IR::ListExpression>();
    auto list =
        new IR::HashListExpression(flc->srcInfo, listExp->components, flc->name, flc->output_width);
    list->fieldListNames = flc->input;
    if (flc->algorithm->names.size() > 0) list->algorithms = flc->algorithm;

    block->push_back(new IR::MethodCallStatement(new IR::MethodCallExpression(
        flc->srcInfo, structure->v1model.hash.Id(),
        {new IR::Argument(new IR::PathExpression(new IR::Path(temp))),
         new IR::Argument(structure->convertHashAlgorithms(flc->algorithm)),
         new IR::Argument(new IR::Constant(ttype, 0)), new IR::Argument(list),
         new IR::Argument(new IR::Constant(IR::Type_Bits::get(flc->output_width + 1),
                                           1 << flc->output_width))})));
    return temp;
}

CONVERT_PRIMITIVE(count_from_hash) {
    if (primitive->operands.size() != 2) return nullptr;
    ExpressionConverter conv(structure);
    auto ref = primitive->operands.at(0);
    const IR::Counter *counter = nullptr;
    if (auto gr = ref->to<IR::GlobalRef>())
        counter = gr->obj->to<IR::Counter>();
    else if (auto nr = ref->to<IR::PathExpression>())
        counter = structure->counters.get(nr->path->name);
    if (counter == nullptr) {
        error("Expected a counter reference %1%", ref);
        return nullptr;
    }
    auto block = new IR::BlockStatement;
    cstring temp = makeHashCall(structure, block, primitive->operands.at(1));
    if (!temp) return nullptr;
    auto counterref = new IR::PathExpression(structure->counters.get(counter));
    auto method = new IR::Member(counterref, structure->v1model.counter.increment.Id());
    auto arg = new IR::Cast(IR::Type::Bits::get(counter->index_width()),
                            new IR::PathExpression(new IR::Path(temp)));
    block->push_back(
        new IR::MethodCallStatement(primitive->srcInfo, method, {new IR::Argument(arg)}));
    return block;
}

static bool makeMeterExecCall(const Util::SourceInfo &srcInfo, ProgramStructure *structure,
                              IR::BlockStatement *block, const IR::Expression *mref,
                              const IR::Expression *index, const IR::Expression *dest) {
    const IR::Meter *meter = nullptr;
    if (auto gr = mref->to<IR::GlobalRef>())
        meter = gr->obj->to<IR::Meter>();
    else if (auto nr = mref->to<IR::PathExpression>())
        meter = structure->meters.get(nr->path->name);
    if (meter == nullptr) {
        error("Expected a meter reference %1%", mref);
        return false;
    }
    auto meterref = new IR::PathExpression(structure->meters.get(meter));
    auto method = new IR::Member(meterref, structure->v1model.meter.executeMeter.Id());
    auto arg = new IR::Cast(IR::Type::Bits::get(meter->index_width()), index);
    block->push_back(new IR::MethodCallStatement(srcInfo, method,
                                                 {new IR::Argument(arg), new IR::Argument(dest)}));
    return true;
}

CONVERT_PRIMITIVE(execute_meter, 5) {
    if (primitive->operands.size() != 4) return nullptr;
    if (use_v1model()) structure->include("tofino/p4_14_prim.p4"_cs, "-D_TRANSLATE_TO_V1MODEL"_cs);
    ExpressionConverter conv(structure);
    // FIXME -- convert this to a custom primitive so TNA translation can convert
    // FIXME -- it to an execute call on a TNA meter
    return new IR::MethodCallStatement(primitive->srcInfo,
                                       IR::ID(primitive->srcInfo, "execute_meter_with_color"_cs),
                                       {new IR::Argument(conv.convert(primitive->operands.at(0))),
                                        new IR::Argument(conv.convert(primitive->operands.at(1))),
                                        new IR::Argument(conv.convert(primitive->operands.at(2))),
                                        new IR::Argument(conv.convert(primitive->operands.at(3)))});
}

CONVERT_PRIMITIVE(execute_meter_from_hash) {
    if (primitive->operands.size() != 3 && primitive->operands.size() != 4) return nullptr;
    ExpressionConverter conv(structure);
    auto block = new IR::BlockStatement;
    cstring temp = makeHashCall(structure, block, primitive->operands.at(1));
    if (primitive->operands.size() == 3) {
        if (!makeMeterExecCall(primitive->srcInfo, structure, block,
                               conv.convert(primitive->operands.at(0)),
                               new IR::PathExpression(new IR::Path(temp)),
                               conv.convert(primitive->operands.at(2))))
            return nullptr;
    } else {
        // has pre-color, so need TNA specific call
        if (use_v1model())
            structure->include("tofino/p4_14_prim.p4"_cs, "-D_TRANSLATE_TO_V1MODEL"_cs);
        block->push_back(new IR::MethodCallStatement(
            primitive->srcInfo, IR::ID(primitive->srcInfo, "execute_meter_with_color"_cs),
            {new IR::Argument(conv.convert(primitive->operands.at(0))),
             new IR::Argument(new IR::PathExpression(new IR::Path(temp))),
             new IR::Argument(conv.convert(primitive->operands.at(2))),
             new IR::Argument(conv.convert(primitive->operands.at(3)))}));
    }
    return block;
}

CONVERT_PRIMITIVE(execute_meter_from_hash_with_or) {
    if (primitive->operands.size() != 3 && primitive->operands.size() != 4) return nullptr;
    ExpressionConverter conv(structure);
    auto block = new IR::BlockStatement;
    cstring temp = makeHashCall(structure, block, primitive->operands.at(1));
    auto dest = conv.convert(primitive->operands.at(2));
    cstring temp2 = structure->makeUniqueName("temp"_cs);
    block->push_back(new IR::Declaration_Variable(temp2, dest->type));
    if (primitive->operands.size() == 3) {
        if (!makeMeterExecCall(primitive->srcInfo, structure, block,
                               conv.convert(primitive->operands.at(0)),
                               new IR::PathExpression(new IR::Path(temp)),
                               new IR::PathExpression(new IR::Path(temp2))))
            return nullptr;
    } else {
        // has pre-color, so need TNA specific call
        if (use_v1model())
            structure->include("tofino/p4_14_prim.p4"_cs, "-D_TRANSLATE_TO_V1MODEL"_cs);
        block->push_back(new IR::MethodCallStatement(
            primitive->srcInfo, IR::ID(primitive->srcInfo, "execute_meter_with_color"_cs),
            {new IR::Argument(conv.convert(primitive->operands.at(0))),
             new IR::Argument(new IR::PathExpression(new IR::Path(temp))),
             new IR::Argument(new IR::PathExpression(new IR::Path(temp2))),
             new IR::Argument(conv.convert(primitive->operands.at(3)))}));
    }
    block->push_back(new IR::AssignmentStatement(
        dest, new IR::BOr(dest, new IR::PathExpression(new IR::Path(temp2)))));
    return block;
}

CONVERT_PRIMITIVE(execute_meter_with_or) {
    if (primitive->operands.size() != 3 && primitive->operands.size() != 4) return nullptr;
    ExpressionConverter conv(structure);
    auto block = new IR::BlockStatement;
    auto dest = conv.convert(primitive->operands.at(2));
    cstring temp2 = structure->makeUniqueName("temp"_cs);
    block->push_back(new IR::Declaration_Variable(temp2, dest->type));
    if (primitive->operands.size() == 3) {
        if (!makeMeterExecCall(primitive->srcInfo, structure, block,
                               conv.convert(primitive->operands.at(0)),
                               conv.convert(primitive->operands.at(1)),
                               new IR::PathExpression(new IR::Path(temp2))))
            return nullptr;
    } else {
        // has pre-color, so need TNA specific call
        if (use_v1model())
            structure->include("tofino/p4_14_prim.p4"_cs, "-D_TRANSLATE_TO_V1MODEL"_cs);
        block->push_back(new IR::MethodCallStatement(
            primitive->srcInfo, IR::ID(primitive->srcInfo, "execute_meter_with_color"_cs),
            {new IR::Argument(conv.convert(primitive->operands.at(0))),
             new IR::Argument(conv.convert(primitive->operands.at(1))),
             new IR::Argument(new IR::PathExpression(new IR::Path(temp2))),
             new IR::Argument(conv.convert(primitive->operands.at(3)))}));
    }
    block->push_back(new IR::AssignmentStatement(
        dest, new IR::BOr(dest, new IR::PathExpression(new IR::Path(temp2)))));
    return block;
}

CONVERT_PRIMITIVE(invalidate) {
    if (primitive->operands.size() != 1) return nullptr;
    if (use_v1model()) structure->include("tofino/p4_14_prim.p4"_cs, "-D_TRANSLATE_TO_V1MODEL"_cs);
    ExpressionConverter conv(structure);
    auto arg = conv.convert(primitive->operands.at(0));
    return new IR::MethodCallStatement(
        primitive->srcInfo,
        IR::ID(primitive->srcInfo, arg->is<IR::Constant>() ? "invalidate_raw"_cs : "invalidate"_cs),
        {new IR::Argument(arg)});
}

CONVERT_PRIMITIVE(invalidate_digest) {
    if (primitive->operands.size() != 0) return nullptr;
    if (use_v1model()) structure->include("tofino/p4_14_prim.p4"_cs, "-D_TRANSLATE_TO_V1MODEL"_cs);
    ExpressionConverter conv(structure);
    // Since V1model does not understand the Tofino metadata, this is a simple pass through
    // and will be translated to `invalidate(ig_intr_md_for_dprsr.digest_type)` in
    // arch/simple_switch.cpp during Tofino mapping.
    return new IR::MethodCallStatement(primitive->srcInfo,
                                       IR::ID(primitive->srcInfo, "invalidate_digest"_cs), {});
}

static const IR::Statement *convertRecirculate(ProgramStructure *structure,
                                               const IR::Primitive *primitive) {
    if (primitive->operands.size() != 1) return nullptr;
    if (use_v1model()) structure->include("tofino/p4_14_prim.p4"_cs, "-D_TRANSLATE_TO_V1MODEL"_cs);
    ExpressionConverter conv(structure);
    auto port = primitive->operands.at(0);
    if (!port->is<IR::Constant>() && !port->is<IR::ActionArg>()) return nullptr;
    port = conv.convert(port);
    port = new IR::Cast(IR::Type::Bits::get(9), port);
    auto recirc = new IR::MethodCallStatement(primitive->srcInfo, "recirculate_raw"_cs,
                                              {new IR::Argument(port)});
    return recirc;
}

CONVERT_PRIMITIVE(recirculate, 5) { return convertRecirculate(structure, primitive); }

CONVERT_PRIMITIVE(recirculate_preserving_field_list, 5) {
    return convertRecirculate(structure, primitive);
}

// This function is identical to the one in frontend in all aspects except that
// the field list used as argument after conversion is a modified class of
// ListExpression. This class saves the field list calculation and field list
// names which are then used in context json to generate PD
CONVERT_PRIMITIVE(modify_field_with_hash_based_offset, 1) {
    ExpressionConverter conv(structure);
    if (primitive->operands.size() != 4) return nullptr;

    auto dest = conv.convert(primitive->operands.at(0));
    auto base = conv.convert(primitive->operands.at(1));
    auto max = conv.convert(primitive->operands.at(3));
    auto args = new IR::Vector<IR::Argument>();

    auto flc = structure->getFieldListCalculation(primitive->operands.at(2));
    if (flc == nullptr) {
        error("%1%: Expected a field_list_calculation", primitive->operands.at(2));
        return nullptr;
    }
    auto ttype = IR::Type_Bits::get(flc->output_width);
    auto fl = structure->getFieldLists(flc);
    if (fl == nullptr) return nullptr;
    const IR::ListExpression *listExp = conv.convert(fl)->to<IR::ListExpression>();
    auto list =
        new IR::HashListExpression(flc->srcInfo, listExp->components, flc->name, flc->output_width);
    list->fieldListNames = flc->input;
    if (flc->algorithm->names.size() > 0) list->algorithms = flc->algorithm;

    auto algorithm = structure->convertHashAlgorithms(flc->algorithm);
    args->push_back(new IR::Argument(dest));
    args->push_back(new IR::Argument(algorithm));
    args->push_back(new IR::Argument(new IR::Cast(ttype, base)));
    args->push_back(new IR::Argument(list));
    args->push_back(new IR::Argument(
        new IR::Cast(max->srcInfo, IR::Type_Bits::get(2 * flc->output_width), max)));
    auto hash = new IR::PathExpression(structure->v1model.hash.Id());
    auto mc = new IR::MethodCallExpression(primitive->srcInfo, hash, args);
    auto result = new IR::MethodCallStatement(primitive->srcInfo, mc);

    auto annotations = new IR::Annotations();
    for (auto annot : fl->annotations->annotations) annotations->annotations.push_back(annot);
    auto block = new IR::BlockStatement(annotations);
    block->components.push_back(result);

    return block;
}

CONVERT_PRIMITIVE(generate_digest, 1) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 2);
    auto args = new IR::Vector<IR::Argument>();
    auto receiver = conv.convert(primitive->operands.at(0));
    args->push_back(
        new IR::Argument(new IR::Cast(primitive->operands.at(0)->srcInfo,
                                      structure->v1model.digest_receiver.receiverType, receiver)));
    auto list = structure->convertFieldList(primitive->operands.at(1));
    auto type = structure->createFieldListType(primitive->operands.at(1));
    structure->types.emplace(type);

    IR::PathExpression *path = new IR::PathExpression("ig_intr_md_for_dprsr");
    auto mem = new IR::Member(path, "digest_type"_cs);
    auto st = dynamic_cast<TnaProgramStructure *>(structure);
    if (st == nullptr) return nullptr;
    unsigned digestId = getIndex(list, st->digestIndexHashes);
    if (digestId > Device::maxDigestId()) {
        error("Too many digest() calls in %1%", primitive->name);
        return nullptr;
    }
    unsigned bits = static_cast<unsigned>(std::ceil(std::log2(Device::maxDigestId())));
    auto idx = new IR::Constant(IR::Type::Bits::get(bits), digestId);
    auto stmt = new IR::AssignmentStatement(mem, idx);
    return stmt;
}

CONVERT_PRIMITIVE(sample_e2e) {
    /* -- the primitive has 3 arguments:
     *    1) (mandatory) mirror session id
     *    2) (mandatory) coalescing length
     *    3) (optional) coalescing header */
    if (primitive->operands.size() < 2 || primitive->operands.size() > 3) return nullptr;

    /* -- add includes which imports the sample3 and sample4 directives */
    if (use_v1model()) structure->include("tofino/p4_14_prim.p4"_cs, "-D_TRANSLATE_TO_V1MODEL"_cs);

    ExpressionConverter conv(structure);

    /* -- the P4-14 sample_e2e primitive is converted to non-oficial primitives
     *    sample3 or sample4:
     *       extern void sample3(in CloneType type, in bit<32> session, in bit<32> length);
     *       extern void sample4<T>(in CloneType type, in bit<32> session, in bit<32> length, in T
     * data);
     *
     *    Construct their argument list now. */
    auto args(new IR::Vector<IR::Argument>());

    /* -- always clone type egress-to-egress */
    auto enumref(new IR::TypeNameExpression(
        new IR::Type_Name(new IR::Path(structure->v1model.clone.cloneType.Id()))));
    auto kindarg(new IR::Member(enumref, structure->v1model.clone.cloneType.e2e.Id()));
    args->push_back(new IR::Argument(kindarg));

    /* -- get the session id directly from sample_e2e arguments */
    auto session(conv.convert(primitive->operands.at(0)));
    args->push_back(new IR::Argument(new IR::Cast(primitive->operands.at(0)->srcInfo,
                                                  structure->v1model.clone.sessionType, session)));

    /* -- coalescing length is in bytes or it's in words if the argument is
     *    an action parameter. */
    auto length(conv.convert(primitive->operands.at(1)));
    args->push_back(new IR::Argument(length));

    IR::ID fn;
    if (primitive->operands.size() == 3) {
        /* -- Handle the optional coalescing header. The header may be a field list
         *    or it may be a header. */
        auto coalesce_hdr(primitive->operands.at(2));
        if (coalesce_hdr->is<IR::ConcreteHeaderRef>()) {
            args->push_back(new IR::Argument(conv.convert(coalesce_hdr)));
        } else {
            auto field_list(structure->convertFieldList(primitive->operands.at(2)));
            if (field_list != nullptr) args->push_back(new IR::Argument(field_list));
        }

        fn = IR::ID(primitive->srcInfo, "sample4"_cs);
    } else {
        fn = IR::ID(primitive->srcInfo, "sample3"_cs);
    }

    return new IR::MethodCallStatement(
        new IR::MethodCallExpression(fn.srcInfo, new IR::PathExpression(fn), args));
}

CONVERT_PRIMITIVE(swap) {
    if (primitive->operands.size() != 2) return nullptr;
    ExpressionConverter conv(structure);
    auto temp = IR::ID(structure->makeUniqueName("temp"_cs));
    auto v1 = primitive->operands.at(0);
    auto v2 = primitive->operands.at(1);
    auto type = v1->type;
    return new IR::BlockStatement(
        {new IR::Declaration_Variable(temp, type, conv.convert(v1)),
         structure->assign(primitive->srcInfo, conv.convert(v1), conv.convert(v2), type),
         structure->assign(primitive->srcInfo, conv.convert(v2), new IR::PathExpression(temp),
                           type)});
}

}  // end namespace P4V1
}  // end namespace P4
