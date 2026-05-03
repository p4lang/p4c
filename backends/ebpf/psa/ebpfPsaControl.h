/*
 * SPDX-FileCopyrightText: 2022 Open Networking Foundation
 * SPDX-FileCopyrightText: 2022 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACKENDS_EBPF_PSA_EBPFPSACONTROL_H_
#define BACKENDS_EBPF_PSA_EBPFPSACONTROL_H_

#include "backends/ebpf/ebpfControl.h"
#include "backends/ebpf/psa/externs/ebpfPsaChecksum.h"
#include "backends/ebpf/psa/externs/ebpfPsaRandom.h"
#include "backends/ebpf/psa/externs/ebpfPsaRegister.h"
#include "ebpfPsaTable.h"

namespace P4::EBPF {

class EBPFControlPSA;

class ControlBodyTranslatorPSA : public ControlBodyTranslator {
 public:
    explicit ControlBodyTranslatorPSA(const EBPFControlPSA *control);

    bool preorder(const IR::BaseAssignmentStatement *a) override { return notSupported(a); }
    bool preorder(const IR::AssignmentStatement *a) override;

    void processMethod(const P4::ExternMethod *method) override;

    virtual cstring getParamName(const IR::PathExpression *);
};

class ActionTranslationVisitorPSA : public ActionTranslationVisitor,
                                    public ControlBodyTranslatorPSA {
 protected:
    const EBPFTablePSA *table;

 public:
    ActionTranslationVisitorPSA(const EBPFProgram *program, cstring valueName,
                                const EBPFTablePSA *table);

    bool preorder(const IR::PathExpression *pe) override;
    bool isActionParameter(const IR::Expression *expression) const;

    void processMethod(const P4::ExternMethod *method) override;
    cstring getParamInstanceName(const IR::Expression *expression) const override;
    cstring getParamName(const IR::PathExpression *) override;
};

class EBPFControlPSA : public EBPFControl {
 public:
    /// Keeps track if ingress_timestamp or egress_timestamp is used within a control block.
    bool timestampIsUsed = false;

    const IR::Parameter *user_metadata;
    const IR::Parameter *inputStandardMetadata;
    const IR::Parameter *outputStandardMetadata;

    std::map<cstring, EBPFHashPSA *> hashes;
    std::map<cstring, EBPFRandomPSA *> randoms;
    std::map<cstring, EBPFRegisterPSA *> registers;
    std::map<cstring, EBPFMeterPSA *> meters;

    EBPFControlPSA(const EBPFProgram *program, const IR::ControlBlock *control,
                   const IR::Parameter *parserHeaders)
        : EBPFControl(program, control, parserHeaders) {}

    void emit(CodeBuilder *builder) override;
    void emitTableTypes(CodeBuilder *builder) override;
    void emitTableInstances(CodeBuilder *builder) override;
    void emitTableInitializers(CodeBuilder *builder) override;

    EBPFRandomPSA *getRandomExt(cstring name) const {
        auto result = ::P4::get(randoms, name);
        BUG_CHECK(result != nullptr, "No random generator named %1%", name);
        return result;
    }

    EBPFRegisterPSA *getRegister(cstring name) const {
        auto result = ::P4::get(registers, name);
        BUG_CHECK(result != nullptr, "No register named %1%", name);
        return result;
    }

    EBPFHashPSA *getHash(cstring name) const {
        auto result = ::P4::get(hashes, name);
        BUG_CHECK(result != nullptr, "No hash named %1%", name);
        return result;
    }

    EBPFMeterPSA *getMeter(cstring name) const {
        auto result = ::P4::get(meters, name);
        BUG_CHECK(result != nullptr, "No meter named %1%", name);
        return result;
    }

    DECLARE_TYPEINFO(EBPFControlPSA, EBPFControl);
};

}  // namespace P4::EBPF

#endif /* BACKENDS_EBPF_PSA_EBPFPSACONTROL_H_ */
