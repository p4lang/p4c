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

#include "lib/bitvec.h"
#include "check_extern_invocation.h"
#include "bf-p4c/device.h"

namespace BFN {

void CheckExternInvocationCommon::checkExtern(const P4::ExternMethod *extMethod,
        const IR::MethodCallExpression *expr) {
    cstring externName = extMethod->object->getName().name;
    cstring externType = extMethod->originalExternType->name;
    bitvec pos;

    if (auto p = findContext<IR::BFN::TnaParser>()) {
        pos.setbit(genIndex(p->thread, PARSER));
        checkPipeConstraints(externType, pos, expr, externName, p->name);
    } else if (auto m = findContext<IR::BFN::TnaControl>()) {
        pos.setbit(genIndex(m->thread, MAU));
        checkPipeConstraints(externType, pos, expr, externName, m->name);
    } else if (auto d = findContext<IR::BFN::TnaDeparser>()) {
        pos.setbit(genIndex(d->thread, DEPARSER));
        checkPipeConstraints(externType, pos, expr, externName, d->name);
    }
}

void CheckExternInvocationCommon::initCommonPipeConstraints() {
    bitvec validInMau;
    validInMau.setbit(genIndex(INGRESS, MAU));
    validInMau.setbit(genIndex(EGRESS, MAU));
    setPipeConstraints("ActionProfile"_cs, validInMau);
    setPipeConstraints("ActionSelector"_cs, validInMau);
    setPipeConstraints("Alpm"_cs, validInMau);
    setPipeConstraints("Atcam"_cs, validInMau);
    setPipeConstraints("Counter"_cs, validInMau);
    setPipeConstraints("DirectCounter"_cs, validInMau);
    setPipeConstraints("DirectLpf"_cs, validInMau);
    setPipeConstraints("DirectMeter"_cs, validInMau);
    setPipeConstraints("DirectRegister"_cs, validInMau);
    setPipeConstraints("DirectRegisterAction"_cs, validInMau);
    setPipeConstraints("DirectWred"_cs, validInMau);
    setPipeConstraints("Hash"_cs, validInMau);
    setPipeConstraints("Lpf"_cs, validInMau);
    setPipeConstraints("MathUnit"_cs, validInMau);
    setPipeConstraints("Meter"_cs, validInMau);
    setPipeConstraints("Random"_cs, validInMau);
    setPipeConstraints("Register"_cs, validInMau);
    setPipeConstraints("RegisterAction"_cs, validInMau);
    setPipeConstraints("RegisterParam"_cs, validInMau);
    setPipeConstraints("Wred"_cs, validInMau);
    setPipeConstraints("invalidate"_cs, validInMau);
    setPipeConstraints("is_validated"_cs, validInMau);
    setPipeConstraints("max"_cs, validInMau);
    setPipeConstraints("min"_cs, validInMau);
    setPipeConstraints("SelectorAction"_cs, validInMau);

    bitvec validInDeparsers;
    validInDeparsers.setbit(genIndex(INGRESS, DEPARSER));
    validInDeparsers.setbit(genIndex(EGRESS, DEPARSER));
    setPipeConstraints("Checksum"_cs, validInDeparsers);
    setPipeConstraints("Digest"_cs, validInDeparsers);
    setPipeConstraints("Mirror"_cs, validInDeparsers);
    setPipeConstraints("Resubmit"_cs, validInDeparsers);
    setPipeConstraints("packet_out"_cs, validInDeparsers);
    setPipeConstraints("is_validated"_cs, validInDeparsers);

    bitvec validInParsers;
    validInParsers.setbit(genIndex(INGRESS, PARSER));
    validInParsers.setbit(genIndex(EGRESS, PARSER));
    setPipeConstraints("Checksum"_cs, validInParsers);
    setPipeConstraints("ParserCounter"_cs, validInParsers);
    setPipeConstraints("ParserPriority"_cs, validInParsers);
    setPipeConstraints("packet_in"_cs, validInParsers);
}

void CheckTNAExternInvocation::initPipeConstraints() {
    initCommonPipeConstraints();
}

void CheckT2NAExternInvocation::initPipeConstraints() {
    initCommonPipeConstraints();

    bitvec validInMau;
    validInMau.setbit(genIndex(INGRESS, MAU));
    validInMau.setbit(genIndex(EGRESS, MAU));
    setPipeConstraints("LearnAction"_cs, validInMau);
    setPipeConstraints("LearnAction2"_cs, validInMau);
    setPipeConstraints("LearnAction3"_cs, validInMau);
    setPipeConstraints("LearnAction4"_cs, validInMau);
    setPipeConstraints("MinMaxAction"_cs, validInMau);
    setPipeConstraints("MinMaxAction2"_cs, validInMau);
    setPipeConstraints("MinMaxAction3"_cs, validInMau);
    setPipeConstraints("MinMaxAction4"_cs, validInMau);

    bitvec validInGhost;
    validInGhost.setbit(genIndex(GHOST, MAU));
    setPipeConstraints("ActionProfile"_cs, validInGhost);
    setPipeConstraints("ActionSelector"_cs, validInGhost);
    setPipeConstraints("Counter"_cs, validInGhost);
    setPipeConstraints("DirectCounter"_cs, validInGhost);
    setPipeConstraints("DirectLpf"_cs, validInGhost);
    setPipeConstraints("DirectMeter"_cs, validInGhost);
    setPipeConstraints("DirectRegister"_cs, validInGhost);
    setPipeConstraints("DirectRegisterAction"_cs, validInGhost);
    setPipeConstraints("DirectRegisterAction2"_cs, validInMau | validInGhost);
    setPipeConstraints("DirectRegisterAction3"_cs, validInMau | validInGhost);
    setPipeConstraints("DirectRegisterAction4"_cs, validInMau | validInGhost);
    setPipeConstraints("DirectWred"_cs, validInGhost);
    setPipeConstraints("SelectorAction"_cs, validInGhost);
    setPipeConstraints("Hash"_cs, validInGhost);
    setPipeConstraints("Lpf"_cs, validInGhost);
    setPipeConstraints("MathUnit"_cs, validInGhost);
    setPipeConstraints("Meter"_cs, validInGhost);
    setPipeConstraints("Random"_cs, validInGhost);
    setPipeConstraints("Register"_cs, validInGhost);
    setPipeConstraints("RegisterAction"_cs, validInGhost);
    setPipeConstraints("RegisterAction2"_cs, validInMau | validInGhost);
    setPipeConstraints("RegisterAction3"_cs, validInMau | validInGhost);
    setPipeConstraints("RegisterAction4"_cs, validInMau | validInGhost);
    setPipeConstraints("RegisterParam"_cs, validInGhost);
    setPipeConstraints("Wred"_cs, validInGhost);
    setPipeConstraints("invalidate"_cs, validInGhost);
    setPipeConstraints("max"_cs, validInGhost);
    setPipeConstraints("min"_cs, validInGhost);

    bitvec validInIngressDeparser;
    validInIngressDeparser.setbit(genIndex(INGRESS, DEPARSER));
    setPipeConstraints("Pktgen"_cs, validInIngressDeparser);

}

}  // namespace BFN
