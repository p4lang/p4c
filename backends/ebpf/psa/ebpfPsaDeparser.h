#ifndef BACKENDS_EBPF_PSA_EBPFPSADEPARSER_H_
#define BACKENDS_EBPF_PSA_EBPFPSADEPARSER_H_

#include "backends/ebpf/ebpfDeparser.h"
#include "ebpfPsaControl.h"
#include "backends/ebpf/psa/ebpfPsaParser.h"

namespace EBPF {

class EBPFDeparserPSA;

class DeparserBodyTranslatorPSA : public DeparserBodyTranslator {
 public:
    explicit DeparserBodyTranslatorPSA(const EBPFDeparserPSA* deparser);

    void processFunction(const P4::ExternFunction* function) override;
};

class EBPFDeparserPSA : public EBPFDeparser {
 public:
    const IR::Parameter* user_metadata;
    const IR::Parameter* istd;
    const IR::Parameter* resubmit_meta;

    EBPFDeparserPSA(const EBPFProgram* program, const IR::ControlBlock* control,
                    const IR::Parameter* parserHeaders, const IR::Parameter *istd) :
            EBPFDeparser(program, control, parserHeaders), istd(istd) {
        codeGen = new DeparserBodyTranslatorPSA(this);
    }

    void emitDeclaration(CodeBuilder* builder, const IR::Declaration* decl) override;
};

class IngressDeparserPSA : public EBPFDeparserPSA {
 public:
    IngressDeparserPSA(const EBPFProgram *program, const IR::ControlBlock *control,
                         const IR::Parameter *parserHeaders, const IR::Parameter *istd) :
            EBPFDeparserPSA(program, control, parserHeaders, istd) {}

    bool build() override;
};

class EgressDeparserPSA : public EBPFDeparserPSA {
 public:
    EgressDeparserPSA(const EBPFProgram *program, const IR::ControlBlock *control,
                      const IR::Parameter *parserHeaders, const IR::Parameter *istd) :
            EBPFDeparserPSA(program, control, parserHeaders, istd) {}

    bool build() override;
};

class TCIngressDeparserPSA : public IngressDeparserPSA {
 public:
    TCIngressDeparserPSA(const EBPFProgram *program, const IR::ControlBlock *control,
                         const IR::Parameter *parserHeaders, const IR::Parameter *istd) :
            IngressDeparserPSA(program, control, parserHeaders, istd) {}

    void emitPreDeparser(CodeBuilder *builder) override;
};

class TCEgressDeparserPSA : public EgressDeparserPSA {
 public:
    TCEgressDeparserPSA(const EBPFProgram *program, const IR::ControlBlock *control,
                          const IR::Parameter *parserHeaders, const IR::Parameter *istd) :
            EgressDeparserPSA(program, control, parserHeaders, istd) { }
};
}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_EBPFPSADEPARSER_H_ */
