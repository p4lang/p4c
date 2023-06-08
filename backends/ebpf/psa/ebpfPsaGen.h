/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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
#ifndef BACKENDS_EBPF_PSA_EBPFPSAGEN_H_
#define BACKENDS_EBPF_PSA_EBPFPSAGEN_H_

#include "backends/bmv2/psa_switch/psaSwitch.h"
#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfObject.h"
#include "backends/ebpf/ebpfOptions.h"
#include "ebpfPipeline.h"
#include "ebpfPsaParser.h"
#include "xdpHelpProgram.h"

namespace EBPF {

enum pipeline_type { TC_INGRESS, TC_EGRESS, XDP_INGRESS, XDP_EGRESS, TC_TRAFFIC_MANAGER };

class EbpfCodeGenerator {
 public:
    const EbpfOptions &options;
    std::vector<EBPF::EBPFType *> ebpfTypes;

    EbpfCodeGenerator(const EbpfOptions &options, std::vector<EBPF::EBPFType *> &ebpfTypes)
        : options(options), ebpfTypes(ebpfTypes) {}

    virtual void emitCommonPreamble(EBPF::CodeBuilder *builder) const = 0;
    virtual void emitPreamble(CodeBuilder *builder) const = 0;
    virtual void emitInternalStructures(EBPF::CodeBuilder *pBuilder) const = 0;
    virtual void emitTypes(EBPF::CodeBuilder *builder) const = 0;
    virtual void emitGlobalHeadersMetadata(EBPF::CodeBuilder *builder) const = 0;
    virtual void emitPipelineInstances(EBPF::CodeBuilder *builder) const = 0;
};

class PSAEbpfGenerator : public EbpfCodeGenerator {
 public:
    static const unsigned MaxClones = 64;
    static const unsigned MaxCloneSessions = 1024;

    EBPFPipeline *ingress;
    EBPFPipeline *egress;

    PSAEbpfGenerator(const EbpfOptions &options, std::vector<EBPFType *> &ebpfTypes,
                     EBPFPipeline *ingress, EBPFPipeline *egress)
        : EbpfCodeGenerator(options, ebpfTypes), ingress(ingress), egress(egress) {}

    virtual void emit(CodeBuilder *builder) const = 0;
    virtual void emitInstances(EBPF::CodeBuilder *builder) const = 0;

    void emitPSAIncludes(CodeBuilder *builder) const;
    void emitPreamble(CodeBuilder *builder) const override;
    void emitCommonPreamble(CodeBuilder *builder) const override;
    void emitInternalStructures(CodeBuilder *pBuilder) const override;
    void emitTypes(CodeBuilder *builder) const override;
    void emitGlobalHeadersMetadata(CodeBuilder *builder) const override;
    void emitPacketReplicationTables(CodeBuilder *builder) const;
    void emitPipelineInstances(CodeBuilder *builder) const override;
    void emitInitializer(CodeBuilder *builder) const;
    virtual void emitInitializerSection(CodeBuilder *builder) const = 0;
    void emitHelperFunctions(CodeBuilder *builder) const;

    // TODO: move them to the externs/ebpfPsaHashAlgorithm.cpp file
    void emitCRC32LookupTableTypes(CodeBuilder *builder) const;
    void emitCRC32LookupTableInitializer(CodeBuilder *builder) const;
    void emitCRC32LookupTableInstance(CodeBuilder *builder) const;
};

class PSAArchTC : public PSAEbpfGenerator {
 public:
    XDPHelpProgram *xdp;

    PSAArchTC(const EbpfOptions &options, std::vector<EBPFType *> &ebpfTypes, XDPHelpProgram *xdp,
              EBPFPipeline *tcIngress, EBPFPipeline *tcEgress)
        : PSAEbpfGenerator(options, ebpfTypes, tcIngress, tcEgress), xdp(xdp) {}

    void emit(CodeBuilder *builder) const override;

    void emitInstances(CodeBuilder *builder) const override;
    void emitInitializerSection(CodeBuilder *builder) const override;
};

class PSAArchXDP : public PSAEbpfGenerator {
 public:
    // TC Ingress program used to support packet cloning in the XDP mode.
    EBPFPipeline *tcIngressForXDP;
    // If the XDP mode is used, we need to have TC Egress pipeline to handle cloned packets.
    EBPFPipeline *tcEgressForXDP;
    static const unsigned egressDevmapSize = 256;

    PSAArchXDP(const EbpfOptions &options, std::vector<EBPFType *> &ebpfTypes,
               EBPFPipeline *xdpIngress, EBPFPipeline *xdpEgress, EBPFPipeline *tcTrafficManager,
               EBPFPipeline *tcEgress)
        : PSAEbpfGenerator(options, ebpfTypes, xdpIngress, xdpEgress),
          tcIngressForXDP(tcTrafficManager),
          tcEgressForXDP(tcEgress) {}

    void emit(CodeBuilder *builder) const override;

    void emitPreamble(CodeBuilder *builder) const override;
    void emitInstances(CodeBuilder *builder) const override;
    void emitInitializerSection(CodeBuilder *builder) const override;

    void emitXDP2TCInternalStructures(CodeBuilder *builder) const;
    void emitDummyProgram(CodeBuilder *builder) const;
};

class ConvertToEbpfPSA : public Transform {
    const EbpfOptions &options;
    P4::TypeMap *typemap;
    P4::ReferenceMap *refmap;
    const PSAEbpfGenerator *ebpf_psa_arch;

 public:
    ConvertToEbpfPSA(const EbpfOptions &options, P4::ReferenceMap *refmap, P4::TypeMap *typemap)
        : options(options), typemap(typemap), refmap(refmap), ebpf_psa_arch(nullptr) {}

    const PSAEbpfGenerator *build(const IR::ToplevelBlock *prog);
    const IR::Node *preorder(IR::ToplevelBlock *p) override;

    const PSAEbpfGenerator *getPSAArchForEBPF() { return ebpf_psa_arch; }
};

class ConvertToEbpfPipeline : public Inspector {
    const cstring name;
    const pipeline_type type;
    const EbpfOptions &options;
    const IR::ParserBlock *parserBlock;
    const IR::ControlBlock *controlBlock;
    const IR::ControlBlock *deparserBlock;
    P4::TypeMap *typemap;
    P4::ReferenceMap *refmap;
    EBPFPipeline *pipeline;

 public:
    ConvertToEbpfPipeline(cstring name, pipeline_type type, const EbpfOptions &options,
                          const IR::ParserBlock *parserBlock, const IR::ControlBlock *controlBlock,
                          const IR::ControlBlock *deparserBlock, P4::ReferenceMap *refmap,
                          P4::TypeMap *typemap)
        : name(name),
          type(type),
          options(options),
          parserBlock(parserBlock),
          controlBlock(controlBlock),
          deparserBlock(deparserBlock),
          typemap(typemap),
          refmap(refmap),
          pipeline(nullptr) {}

    bool preorder(const IR::PackageBlock *block) override;
    EBPFPipeline *getEbpfPipeline() { return pipeline; }
};

class ConvertToEBPFParserPSA : public Inspector {
    EBPF::EBPFProgram *program;
    pipeline_type type;

    P4::TypeMap *typemap;
    EBPF::EBPFPsaParser *parser;

 public:
    ConvertToEBPFParserPSA(EBPF::EBPFProgram *program, P4::TypeMap *typemap, pipeline_type type)
        : program(program), type(type), typemap(typemap), parser(nullptr) {}

    bool preorder(const IR::ParserBlock *prsr) override;
    bool preorder(const IR::P4ValueSet *pvs) override;
    EBPF::EBPFParser *getEBPFParser() { return parser; }
};

class ConvertToEBPFControlPSA : public Inspector {
    EBPF::EBPFProgram *program;
    pipeline_type type;
    EBPF::EBPFControlPSA *control;

    const IR::Parameter *parserHeaders;
    P4::ReferenceMap *refmap;

 public:
    ConvertToEBPFControlPSA(EBPF::EBPFProgram *program, const IR::Parameter *parserHeaders,
                            P4::ReferenceMap *refmap, pipeline_type type)
        : program(program),
          type(type),
          control(nullptr),
          parserHeaders(parserHeaders),
          refmap(refmap) {}

    bool preorder(const IR::TableBlock *) override;
    bool preorder(const IR::ControlBlock *) override;
    bool preorder(const IR::Declaration_Variable *) override;
    bool preorder(const IR::Member *m) override;
    bool preorder(const IR::IfStatement *a) override;
    bool preorder(const IR::ExternBlock *instance) override;

    EBPF::EBPFControlPSA *getEBPFControl() { return control; }
};

class ConvertToEBPFDeparserPSA : public Inspector {
    EBPF::EBPFProgram *program;
    pipeline_type pipelineType;

    const IR::Parameter *parserHeaders;
    const IR::Parameter *istd;
    EBPF::EBPFDeparserPSA *deparser;

 public:
    ConvertToEBPFDeparserPSA(EBPFProgram *program, const IR::Parameter *parserHeaders,
                             const IR::Parameter *istd, pipeline_type type)
        : program(program),
          pipelineType(type),
          parserHeaders(parserHeaders),
          istd(istd),
          deparser(nullptr) {}

    bool preorder(const IR::ControlBlock *) override;
    bool preorder(const IR::Declaration_Instance *) override;
    EBPF::EBPFDeparserPSA *getEBPFDeparser() { return deparser; }
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_EBPFPSAGEN_H_ */
