#ifndef BACKENDS_EBPF_PSA_EBPFPSACONTROL_H_
#define BACKENDS_EBPF_PSA_EBPFPSACONTROL_H_

#include "ebpfPsaTable.h"
#include "backends/ebpf/ebpfControl.h"

namespace EBPF {

class EBPFControlPSA;

class EBPFControlPSA : public EBPFControl {
 public:
    bool timestampIsUsed = false;

    const IR::Parameter* user_metadata;
    const IR::Parameter* inputStandardMetadata;
    const IR::Parameter* outputStandardMetadata;

    EBPFControlPSA(const EBPFProgram* program, const IR::ControlBlock* control,
                   const IR::Parameter* parserHeaders) :
        EBPFControl(program, control, parserHeaders) {}
};

}  // namespace EBPF

#endif  /* BACKENDS_EBPF_PSA_EBPFPSACONTROL_H_ */
