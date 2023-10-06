/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#ifndef BACKENDS_TC_EBPFCODEGEN_H_
#define BACKENDS_TC_EBPFCODEGEN_H_

#include "backend.h"

namespace TC {

class ConvertToBackendIR;
class EBPFPnaParser;

//  Similar to class PSAEbpfGenerator in backends/ebpf/psa/ebpfPsaGen.h

class PNAEbpfGenerator : public EBPF::EbpfCodeGenerator {
 public:
    EBPF::EBPFPipeline *pipeline;

    PNAEbpfGenerator(const EbpfOptions &options, std::vector<EBPF::EBPFType *> &ebpfTypes,
                     EBPF::EBPFPipeline *pipeline)
        : EBPF::EbpfCodeGenerator(options, ebpfTypes), pipeline(pipeline) {}

    virtual void emit(EBPF::CodeBuilder *builder) const = 0;
    virtual void emitInstances(EBPF::CodeBuilder *builder) const = 0;
    virtual void emitParser(EBPF::CodeBuilder *builder) const = 0;
    virtual void emitHeader(EBPF::CodeBuilder *builder) const = 0;
    void emitPNAIncludes(EBPF::CodeBuilder *builder) const;
    void emitPreamble(EBPF::CodeBuilder *builder) const override;
    void emitCommonPreamble(EBPF::CodeBuilder *builder) const override;
    void emitInternalStructures(EBPF::CodeBuilder *pBuilder) const override;
    void emitTypes(EBPF::CodeBuilder *builder) const override;
    void emitGlobalHeadersMetadata(EBPF::CodeBuilder *builder) const override;
    void emitPipelineInstances(EBPF::CodeBuilder *builder) const override;
    cstring getProgramName() const;
};

// Similar to class PSAErrorCodesGen in backends/ebpf/psa/ebpfPsaGen.cpp

class PNAErrorCodesGen : public Inspector {
    EBPF::CodeBuilder *builder;

 public:
    explicit PNAErrorCodesGen(EBPF::CodeBuilder *builder) : builder(builder) {}

    bool preorder(const IR::Type_Error *errors) override {
        int id = -1;
        for (auto decl : errors->members) {
            ++id;
            if (decl->srcInfo.isValid()) {
                auto sourceFile = decl->srcInfo.getSourceFile();
                // all the error codes are located in core.p4 file, they are defined in pna.h
                if (sourceFile.endsWith("p4include/core.p4")) continue;
            }

            builder->emitIndent();
            builder->appendFormat("static const ParserError_t %s = %d", decl->name.name, id);
            builder->endOfStatement(true);

            // type ParserError_t is u8, which can have values from 0 to 255
            if (id > 255) {
                ::error(ErrorType::ERR_OVERLIMIT, "%1%: Reached maximum number of possible errors",
                        decl);
            }
        }
        builder->newline();
        return false;
    }
};

//  Similar to class PSAArchTC in backends/ebpf/psa/ebpfPsaGen.h

class PNAArchTC : public PNAEbpfGenerator {
 public:
    EBPF::XDPHelpProgram *xdp;

    PNAArchTC(const EbpfOptions &options, std::vector<EBPF::EBPFType *> &ebpfTypes,
              EBPF::XDPHelpProgram *xdp, EBPF::EBPFPipeline *pipeline)
        : PNAEbpfGenerator(options, ebpfTypes, pipeline), xdp(xdp) {}

    void emit(EBPF::CodeBuilder *builder) const override;
    void emitParser(EBPF::CodeBuilder *builder) const override;
    void emitHeader(EBPF::CodeBuilder *builder) const override;
    void emitInstances(EBPF::CodeBuilder *builder) const override;
};

class TCIngressPipelinePNA : public EBPF::TCIngressPipeline {
 public:
    TCIngressPipelinePNA(cstring name, const EbpfOptions &options, P4::ReferenceMap *refMap,
                         P4::TypeMap *typeMap)
        : EBPF::TCIngressPipeline(name, options, refMap, typeMap) {}

    void emit(EBPF::CodeBuilder *builder) override;
    void emitLocalVariables(EBPF::CodeBuilder *builder) override;
    void emitGlobalMetadataInitializer(EBPF::CodeBuilder *builder) override;
    void emitTrafficManager(EBPF::CodeBuilder *builder) override;
};

class PnaStateTranslationVisitor : public EBPF::PsaStateTranslationVisitor {
 public:
    explicit PnaStateTranslationVisitor(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                        EBPF::EBPFPsaParser *prsr)
        : EBPF::PsaStateTranslationVisitor(refMap, typeMap, prsr) {}

 protected:
    void compileExtractField(const IR::Expression *expr, const IR::StructField *field,
                             unsigned alignment, EBPF::EBPFType *type) override;
};

class EBPFPnaParser : public EBPF::EBPFPsaParser {
 public:
    EBPFPnaParser(const EBPF::EBPFProgram *program, const IR::ParserBlock *block,
                  const P4::TypeMap *typeMap);
    void emit(EBPF::CodeBuilder *builder) override;
};

class EBPFTablePNA : public EBPF::EBPFTablePSA {
 protected:
    EBPF::ActionTranslationVisitor *createActionTranslationVisitor(
        cstring valueName, const EBPF::EBPFProgram *program) const override;
    const ConvertToBackendIR *tcIR;

 public:
    EBPFTablePNA(const EBPF::EBPFProgram *program, const IR::TableBlock *table,
                 EBPF::CodeGenInspector *codeGen, const ConvertToBackendIR *tcIR)
        : EBPF::EBPFTablePSA(program, table, codeGen), tcIR(tcIR) {}
    void emitInitializer(EBPF::CodeBuilder *builder) override;
    void emitDefaultActionStruct(EBPF::CodeBuilder *builder);
    void emitKeyType(EBPF::CodeBuilder *builder) override;
    void emitValueType(EBPF::CodeBuilder *builder) override;
    void emitValueStructStructure(EBPF::CodeBuilder *builder) override;
    void emitActionArguments(EBPF::CodeBuilder *builder, const IR::P4Action *action, cstring name);
    void emitKeyPNA(EBPF::CodeBuilder *builder, cstring keyName);
    bool isMatchTypeSupported(const IR::Declaration_ID *matchType) override {
        return EBPF::EBPFTable::isMatchTypeSupported(matchType);
    }
    void emitAction(EBPF::CodeBuilder *builder, cstring valueName,
                    cstring actionRunVariable) override;
    void emitValueActionIDNames(EBPF::CodeBuilder *builder) override;
};

class IngressDeparserPNA : public EBPF::EBPFDeparserPSA {
 public:
    IngressDeparserPNA(const EBPF::EBPFProgram *program, const IR::ControlBlock *control,
                       const IR::Parameter *parserHeaders, const IR::Parameter *istd)
        : EBPF::EBPFDeparserPSA(program, control, parserHeaders, istd) {}

    bool build() override;
    void emit(EBPF::CodeBuilder *builder) override;
    void emitPreDeparser(EBPF::CodeBuilder *builder) override;
};

// Similar to class ConvertToEbpfPSA in backends/ebpf/psa/ebpfPsaGen.h

class ConvertToEbpfPNA : public Transform {
    const EbpfOptions &options;
    P4::TypeMap *typemap;
    P4::ReferenceMap *refmap;
    const PNAEbpfGenerator *ebpf_program;
    const ConvertToBackendIR *tcIR;

 public:
    ConvertToEbpfPNA(const EbpfOptions &options, P4::ReferenceMap *refmap, P4::TypeMap *typemap,
                     const ConvertToBackendIR *tcIR)
        : options(options), typemap(typemap), refmap(refmap), ebpf_program(nullptr), tcIR(tcIR) {}

    const PNAEbpfGenerator *build(const IR::ToplevelBlock *prog);
    const IR::Node *preorder(IR::ToplevelBlock *p) override;

    const PNAEbpfGenerator *getEBPFProgram() { return ebpf_program; }
};

// Similar to class ConvertToEbpfPipeline in backends/ebpf/psa/ebpfPsaGen.h

class ConvertToEbpfPipelineTC : public Inspector {
    const cstring name;
    const EBPF::pipeline_type type;
    const EbpfOptions &options;
    const IR::ParserBlock *parserBlock;
    const IR::ControlBlock *controlBlock;
    const IR::ControlBlock *deparserBlock;
    P4::TypeMap *typemap;
    P4::ReferenceMap *refmap;
    EBPF::EBPFPipeline *pipeline;
    const ConvertToBackendIR *tcIR;

 public:
    ConvertToEbpfPipelineTC(cstring name, EBPF::pipeline_type type, const EbpfOptions &options,
                            const IR::ParserBlock *parserBlock,
                            const IR::ControlBlock *controlBlock,
                            const IR::ControlBlock *deparserBlock, P4::ReferenceMap *refmap,
                            P4::TypeMap *typemap, const ConvertToBackendIR *tcIR)
        : name(name),
          type(type),
          options(options),
          parserBlock(parserBlock),
          controlBlock(controlBlock),
          deparserBlock(deparserBlock),
          typemap(typemap),
          refmap(refmap),
          pipeline(nullptr),
          tcIR(tcIR) {}

    bool preorder(const IR::PackageBlock *block) override;
    EBPF::EBPFPipeline *getEbpfPipeline() { return pipeline; }
};

// Similar to class ConvertToEBPFParserPSA in backends/ebpf/psa/ebpfPsaGen.h

class ConvertToEBPFParserPNA : public Inspector {
    EBPF::EBPFProgram *program;
    P4::TypeMap *typemap;
    TC::EBPFPnaParser *parser;

 public:
    ConvertToEBPFParserPNA(EBPF::EBPFProgram *program, P4::TypeMap *typemap)
        : program(program), typemap(typemap), parser(nullptr) {}

    bool preorder(const IR::ParserBlock *prsr) override;
    bool preorder(const IR::P4ValueSet *pvs) override;
    EBPF::EBPFParser *getEBPFParser() { return parser; }
};

// Similar to class ConvertToEBPFControlPSA in backends/ebpf/psa/ebpfPsaGen.h

class ConvertToEBPFControlPNA : public Inspector {
    EBPF::EBPFProgram *program;
    EBPF::pipeline_type type;
    EBPF::EBPFControlPSA *control;

    const IR::Parameter *parserHeaders;
    P4::ReferenceMap *refmap;

    const ConvertToBackendIR *tcIR;

 public:
    ConvertToEBPFControlPNA(EBPF::EBPFProgram *program, const IR::Parameter *parserHeaders,
                            P4::ReferenceMap *refmap, EBPF::pipeline_type type,
                            const ConvertToBackendIR *tcIR)
        : program(program),
          type(type),
          control(nullptr),
          parserHeaders(parserHeaders),
          refmap(refmap),
          tcIR(tcIR) {}

    bool preorder(const IR::TableBlock *) override;
    bool preorder(const IR::ControlBlock *) override;
    bool preorder(const IR::Declaration_Variable *) override;
    bool preorder(const IR::Member *m) override;
    bool preorder(const IR::IfStatement *a) override;
    bool checkPnaTimestampMem(const IR::Member *m);
    EBPF::EBPFControlPSA *getEBPFControl() { return control; }
};

// Similar to class ConvertToEBPFDeparserPSA in backends/ebpf/psa/ebpfPsaGen.h

class ConvertToEBPFDeparserPNA : public Inspector {
    EBPF::EBPFProgram *program;
    const IR::Parameter *parserHeaders;
    const IR::Parameter *istd;
    TC::IngressDeparserPNA *deparser;

 public:
    ConvertToEBPFDeparserPNA(EBPF::EBPFProgram *program, const IR::Parameter *parserHeaders,
                             const IR::Parameter *istd)
        : program(program), parserHeaders(parserHeaders), istd(istd), deparser(nullptr) {}

    bool preorder(const IR::ControlBlock *) override;
    bool preorder(const IR::Declaration_Instance *) override;
    EBPF::EBPFDeparserPSA *getEBPFDeparser() { return deparser; }
};

// Similar to class ControlBodyTranslatorPSA in backends/ebpf/psa/ebpfPsaControl.h

class ControlBodyTranslatorPNA : public EBPF::ControlBodyTranslator {
 public:
    const ConvertToBackendIR *tcIR;
    explicit ControlBodyTranslatorPNA(const EBPF::EBPFControlPSA *control);
    explicit ControlBodyTranslatorPNA(const EBPF::EBPFControlPSA *control,
                                      const ConvertToBackendIR *tcIR);
    void processFunction(const P4::ExternFunction *function);
    void processApply(const P4::ApplyMethod *method);
    bool checkPnaPortMem(const IR::Member *m);
    virtual cstring getParamName(const IR::PathExpression *);
};

// Similar to class ActionTranslationVisitorPSA in backends/ebpf/psa/ebpfPsaControl.h

class ActionTranslationVisitorPNA : public EBPF::ActionTranslationVisitor,
                                    public ControlBodyTranslatorPNA {
 protected:
    const EBPF::EBPFTablePSA *table;

 public:
    ActionTranslationVisitorPNA(const EBPF::EBPFProgram *program, cstring valueName,
                                const EBPF::EBPFTablePSA *table);

    bool preorder(const IR::PathExpression *pe) override;
    bool isActionParameter(const IR::Expression *expression) const;

    cstring getParamInstanceName(const IR::Expression *expression) const override;
    cstring getParamName(const IR::PathExpression *) override;
};

// Similar to class DeparserHdrEmitTranslator  in backends/ebpf/ebpfDeparser.h

class DeparserHdrEmitTranslatorPNA : public EBPF::DeparserPrepareBufferTranslator {
 protected:
    const EBPF::EBPFDeparser *deparser;

 public:
    explicit DeparserHdrEmitTranslatorPNA(const EBPF::EBPFDeparser *deparser);

    void processMethod(const P4::ExternMethod *method) override;
    void emitField(EBPF::CodeBuilder *builder, cstring field, const IR::Expression *hdrExpr,
                   unsigned alignment, EBPF::EBPFType *type, bool isMAC);
};

}  // namespace TC

#endif /* BACKENDS_TC_EBPFCODEGEN_H_ */
