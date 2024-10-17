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

#include "drop_packet_with_mirror_engine.h"
#include "bf-p4c/device.h"

namespace BFN {

#if 0
const IR::Node* DropPacketWithMirrorEngine_::preorder(IR::BFN::TnaControl *control) {
    // ig_intr_md_for_dprsr.mirror_type is always initialized to zero by parser
    // in tofino1
    if (Device::currentDevice() != Device::JBAY &&
        ) {
        prune();
        return control; }

    if (control->thread != INGRESS) {
        prune();
        return control; }

    // If available, use existing eg_intr_md_for_dprsr metadata parameter
    if (control->tnaParams.find("ig_intr_md_for_dprsr") != control->tnaParams.end()) {
        // Save existing eg_intr_md_for_dprsr parameter name for later use
        igIntrMdForDprsrName = control->tnaParams.at("ig_intr_md_for_dprsr");
    }
    auto args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(
            new IR::Member(new IR::PathExpression(igIntrMdForDprsrName), "mirror_type")));
    auto typeArgs = new IR::Vector<IR::Type>();
    typeArgs->push_back(IR::Type::Bits::get(Device::mirrorTypeWidth()));
    auto check_is_validated_mirror_type = new IR::IfStatement(
        new IR::LNot(
            new IR::MethodCallExpression(
                new IR::PathExpression("is_validated"), typeArgs, args)),
        new IR::AssignmentStatement(
            new IR::Member(new IR::PathExpression(igIntrMdForDprsrName), "mirror_type"),
            new IR::Constant(IR::Type::Bits::get(Device::mirrorTypeWidth()), 0)),
        nullptr);

    // append
    //
    // if (!is_valid(mirror.mirror_type))
    //      mirror.mirror_type = 3w0;
    //
    // to the end of ingress control
    IR::IndexedVector<IR::StatOrDecl> comp;
    comp.append(control->body->components);
    comp.push_back(check_is_validated_mirror_type);
    auto body = new IR::BlockStatement(
        control->body->srcInfo, control->body->getAnnotations(), comp);

    auto rv = new IR::BFN::TnaControl(
        control->srcInfo, control->name, control->type,
        control->constructorParams, control->controlLocals,
        body, control->tnaParams, control->thread, control->pipeName);
    return rv;
}
#endif

const IR::Node* DropPacketWithMirrorEngine_::preorder(IR::Declaration_Instance* inst) {
    unique_names.insert(inst->name);
    return inst;
}

const IR::Node* DropPacketWithMirrorEngine_::postorder(IR::BFN::TnaDeparser *dp) {
    if (dp->thread != INGRESS) {
        prune();
        return dp; }
    // If available, use existing eg_intr_md_for_dprsr metadata parameter
    if (dp->tnaParams.find("ig_intr_md_for_dprsr"_cs) != dp->tnaParams.end()) {
        // Save existing eg_intr_md_for_dprsr parameter name for later use
        igIntrMdForDprsrName = dp->tnaParams.at("ig_intr_md_for_dprsr"_cs);
    }
    // create `Mirror() mirror` constructor call
    auto name = cstring::make_unique(unique_names, "mirror"_cs, '_');
    auto args = new IR::Vector<IR::Argument>();
#if 0
    // FIXME: finalize the syntax for mirror extern. When mirror idx
    // is provided in constructor, do we still support the syntax with
    // if-command. Is the if-command redundant in this example?
    // Mirror(0) mirror;
    // if (mirror_type == 0)
    //    mirror.emit()?
    // Fixup the implementation in extract_deparser.cpp
    args->push_back(new IR::Argument(
        new IR::Constant(IR::Type::Bits::get(Device::mirrorTypeWidth()), 0)));
#endif
    auto constructor =
        new IR::Declaration_Instance(name, new IR::Type_Name("Mirror"), args, nullptr);

    // create mirror.emit(0) method call;
    auto emit_invalid_mirror_session_args = new IR::Vector<IR::Argument>();
    emit_invalid_mirror_session_args->push_back(
        new IR::Argument(new IR::Constant(IR::Type::Bits::get(Device::cloneSessionIdWidth()), 0)));
    auto emit_invalid_mirror_session = new IR::IfStatement(
        new IR::Equ(
            IR::Type::Bits::get(Device::mirrorTypeWidth()),
            new IR::Member(new IR::PathExpression(igIntrMdForDprsrName), "mirror_type"),
            new IR::Constant(IR::Type::Bits::get(Device::mirrorTypeWidth()), 0)),
        new IR::MethodCallStatement({
            new IR::MethodCallExpression(
                new IR::Member(new IR::PathExpression(name), "emit"),
                emit_invalid_mirror_session_args)}),
        nullptr);

    // prepend Mirror() mirror to deparser statements;
    IR::IndexedVector<IR::Declaration> local_vars;
    local_vars.push_back(constructor);
    local_vars.append(dp->controlLocals);

    // prepend mirror.emit() to apply block
    IR::IndexedVector<IR::StatOrDecl> comp;
    comp.push_back(emit_invalid_mirror_session);
    comp.append(dp->body->components);
    auto body = new IR::BlockStatement(dp->body->srcInfo, dp->body->getAnnotations(), comp);

    // create new Deparser
    auto rv = new IR::BFN::TnaDeparser(
            dp->srcInfo, dp->name, dp->type,
            dp->constructorParams, local_vars,
            body, dp->tnaParams, dp->thread, dp->pipeName);
    return rv; }

}  // namespace BFN
