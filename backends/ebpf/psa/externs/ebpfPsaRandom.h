/*
 * SPDX-FileCopyrightText: 2022 Open Networking Foundation
 * SPDX-FileCopyrightText: 2022 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSARANDOM_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSARANDOM_H_

#include "backends/ebpf/ebpfObject.h"
#include "frontends/p4/methodInstance.h"

namespace P4::EBPF {

class EBPFRandomPSA : public EBPFObject {
    unsigned int minValue, maxValue;
    long range;

 public:
    explicit EBPFRandomPSA(const IR::Declaration_Instance *di);

    void processMethod(CodeBuilder *builder, const P4::ExternMethod *method) const;
    void emitRead(CodeBuilder *builder) const;
};

}  // namespace P4::EBPF

#endif  // BACKENDS_EBPF_PSA_EXTERNS_EBPFPSARANDOM_H_
