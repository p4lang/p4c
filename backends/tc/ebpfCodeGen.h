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

// FIXME: these include each other and present file
#include "backend.h"
#include "tcExterns.h"

namespace P4::TC {

using namespace P4::literals;

class ConvertToBackendIR;
class EBPFPnaParser;
class EBPFRegisterPNA;

//  Similar to class PSAEbpfGenerator in backends/ebpf/psa/ebpfPsaGen.h

class PNAEbpfGenerator : public EBPF::EbpfCodeGenerator {
 public:
    EBPF::EBPFPipeline *pipeline;
    const ConvertToBackendIR *tcIR;

    PNAEbpfGenerator(const EbpfOptions &options, std::vector<EBPF::EBPFType *> &ebpfTypes,
                     EBPF::EBPFPipeline *pipeline, const ConvertToBackendIR *tcIR)
        : EBPF::EbpfCodeGenerator(options, ebpfTypes), pipeline(pipeline), tcIR(tcIR) {}

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
    void emitP4TCFilterFields(EBPF::CodeBuilder *builder) const;
    void emitP4TCActionParam(EBPF::CodeBuilder *builder) const;
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
                ::P4::error(ErrorType::ERR_OVERLIMIT,
                            "%1%: Reached maximum number of possible errors", decl);
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
              EBPF::XDPHelpProgram *xdp, EBPF::EBPFPipeline *pipeline,
              const ConvertToBackendIR *tcIR)
        : PNAEbpfGenerator(options, ebpfTypes, pipeline, tcIR), xdp(xdp) {}

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

    DECLARE_TYPEINFO(TCIngressPipelinePNA, EBPF::TCIngressPipeline);
};

class PnaStateTranslationVisitor : public EBPF::PsaStateTranslationVisitor {
 public:
    explicit PnaStateTranslationVisitor(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                        EBPF::EBPFPsaParser *prsr)
        : EBPF::PsaStateTranslationVisitor(refMap, typeMap, prsr) {}

    bool preorder(const IR::Member *expression) override;

 protected:
    void compileExtractField(const IR::Expression *expr, const IR::StructField *field,
                             unsigned hdrOffsetBits, EBPF::EBPFType *type) override;
    void compileLookahead(const IR::Expression *destination) override;
};

class EBPFPnaParser : public EBPF::EBPFPsaParser {
 public:
    EBPFPnaParser(const EBPF::EBPFProgram *program, const IR::ParserBlock *block,
                  const P4::TypeMap *typeMap);
    void emit(EBPF::CodeBuilder *builder) override;
    void emitRejectState(EBPF::CodeBuilder *) override;
    void emitDeclaration(EBPF::CodeBuilder *builder, const IR::Declaration *decl) override;

    DECLARE_TYPEINFO(EBPFPnaParser, EBPF::EBPFPsaParser);
};

class EBPFTablePNA : public EBPF::EBPFTablePSA {
 protected:
    EBPF::ActionTranslationVisitor *createActionTranslationVisitor(cstring valueName,
                                                                   const EBPF::EBPFProgram *program,
                                                                   const IR::P4Action *action,
                                                                   bool isDefaultAction) const;
    void validateKeys() const override;
    void initDirectCounters();
    void initDirectMeters();
    const ConvertToBackendIR *tcIR;

 public:
    EBPFTablePNA(const EBPF::EBPFProgram *program, const IR::TableBlock *table,
                 EBPF::CodeGenInspector *codeGen, const ConvertToBackendIR *tcIR)
        : EBPF::EBPFTablePSA(program, table, codeGen), tcIR(tcIR) {
        initDirectCounters();
        initDirectMeters();
    }
    void emitInitializer(EBPF::CodeBuilder *builder) override;
    void emitDefaultActionStruct(EBPF::CodeBuilder *builder);
    void emitKeyType(EBPF::CodeBuilder *builder) override;
    void emitValueType(EBPF::CodeBuilder *builder) override;
    void emitValueStructStructure(EBPF::CodeBuilder *builder) override;
    void emitActionArguments(EBPF::CodeBuilder *builder, const IR::P4Action *action, cstring name);
    void emitKeyPNA(EBPF::CodeBuilder *builder, cstring keyName);
    bool isMatchTypeSupported(const IR::Declaration_ID *matchType) override {
        if (matchType->name.name == "range" || matchType->name.name == "rangelist" ||
            matchType->name.name == "optional")
            return 1;
        return EBPF::EBPFTable::isMatchTypeSupported(matchType);
    }
    void emitAction(EBPF::CodeBuilder *builder, cstring valueName,
                    cstring actionRunVariable) override;
    void emitValueActionIDNames(EBPF::CodeBuilder *builder) override;
    cstring p4ActionToActionIDName(const IR::P4Action *action) const;

    DECLARE_TYPEINFO(EBPFTablePNA, EBPF::EBPFTablePSA);
};

class IngressDeparserPNA : public EBPF::EBPFDeparserPSA {
 public:
    IngressDeparserPNA(const EBPF::EBPFProgram *program, const IR::ControlBlock *control,
                       const IR::Parameter *parserHeaders, const IR::Parameter *istd)
        : EBPF::EBPFDeparserPSA(program, control, parserHeaders, istd) {}

    bool addExternDeclaration = false;
    bool build() override;
    void emit(EBPF::CodeBuilder *builder) override;
    void emitPreDeparser(EBPF::CodeBuilder *builder) override;
    void emitDeclaration(EBPF::CodeBuilder *builder, const IR::Declaration *decl) override;

    void emitExternDefinition(EBPF::CodeBuilder *builder) {
        if (addExternDeclaration) {
            builder->emitIndent();
            builder->appendLine("struct p4tc_ext_bpf_params ext_params = {};");
        }
    }
    DECLARE_TYPEINFO(IngressDeparserPNA, EBPF::EBPFDeparserPSA);
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

class EBPFControlPNA : public EBPF::EBPFControlPSA {
 public:
    bool addExternDeclaration = false;
    std::map<cstring, EBPFRegisterPNA *> pna_registers;

    EBPFControlPNA(const EBPF::EBPFProgram *program, const IR::ControlBlock *control,
                   const IR::Parameter *parserHeaders)
        : EBPF::EBPFControlPSA(program, control, parserHeaders) {}

    EBPFRegisterPNA *getRegister(cstring name) const {
        auto result = ::P4::get(pna_registers, name);
        BUG_CHECK(result != nullptr, "No register named %1%", name);
        return result;
    }
    void emitExternDefinition(EBPF::CodeBuilder *builder) {
        if (addExternDeclaration) {
            builder->emitIndent();
            builder->appendLine("struct p4tc_ext_bpf_params ext_params = {};");
            builder->emitIndent();
            builder->appendLine("struct p4tc_ext_bpf_val ext_val = {};");
            builder->emitIndent();
            builder->appendLine("struct p4tc_ext_bpf_val *ext_val_ptr;");
        }
    }
    void emitTableTypes(EBPF::CodeBuilder *builder) { EBPF::EBPFControl::emitTableTypes(builder); }
};

// Similar to class ConvertToEBPFControlPSA in backends/ebpf/psa/ebpfPsaGen.h

class ConvertToEBPFControlPNA : public Inspector {
    EBPF::EBPFProgram *program;
    EBPF::pipeline_type type;
    EBPFControlPNA *control;

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
    bool preorder(const IR::ExternBlock *instance) override;
    bool checkPnaTimestampMem(const IR::Member *m);
    EBPFControlPNA *getEBPFControl() { return control; }
};

// Similar to class ConvertToEBPFDeparserPSA in backends/ebpf/psa/ebpfPsaGen.h

class ConvertToEBPFDeparserPNA : public Inspector {
    EBPF::EBPFProgram *program;
    const IR::Parameter *parserHeaders;
    const IR::Parameter *istd;
    const ConvertToBackendIR *tcIR;
    TC::IngressDeparserPNA *deparser;

 public:
    ConvertToEBPFDeparserPNA(EBPF::EBPFProgram *program, const IR::Parameter *parserHeaders,
                             const IR::Parameter *istd, const ConvertToBackendIR *tcIR)
        : program(program),
          parserHeaders(parserHeaders),
          istd(istd),
          tcIR(tcIR),
          deparser(nullptr) {}

    bool preorder(const IR::ControlBlock *) override;
    bool preorder(const IR::Declaration_Instance *) override;
    EBPF::EBPFDeparserPSA *getEBPFDeparser() { return deparser; }
};

// Similar to class ControlBodyTranslatorPSA in backends/ebpf/psa/ebpfPsaControl.h

class ControlBodyTranslatorPNA : public EBPF::ControlBodyTranslator {
 public:
    const ConvertToBackendIR *tcIR;
    const EBPF::EBPFTablePSA *table;
    explicit ControlBodyTranslatorPNA(const EBPFControlPNA *control);
    explicit ControlBodyTranslatorPNA(const EBPFControlPNA *control,
                                      const ConvertToBackendIR *tcIR);
    explicit ControlBodyTranslatorPNA(const EBPFControlPNA *control, const ConvertToBackendIR *tcIR,
                                      const EBPF::EBPFTablePSA *table);
    void processFunction(const P4::ExternFunction *function) override;
    void processApply(const P4::ApplyMethod *method) override;
    virtual cstring getParamName(const IR::PathExpression *);
    bool preorder(const IR::AssignmentStatement *a) override;
    void processMethod(const P4::ExternMethod *method) override;
    bool preorder(const IR::Member *) override;
    bool IsTableAddOnMiss(const IR::P4Table *table);
    const IR::P4Action *GetAddOnMissHitAction(cstring actionName);
    void ValidateAddOnMissMissAction(const IR::P4Action *act);
};

// Similar to class ActionTranslationVisitorPSA in backends/ebpf/psa/ebpfPsaControl.h

class ActionTranslationVisitorPNA : public EBPF::ActionTranslationVisitor,
                                    public ControlBodyTranslatorPNA {
 protected:
    const EBPF::EBPFTablePSA *table;
    bool isDefaultAction;

 public:
    const ConvertToBackendIR *tcIR;
    ActionTranslationVisitorPNA(const EBPF::EBPFProgram *program, cstring valueName,
                                const EBPF::EBPFTablePSA *table, const ConvertToBackendIR *tcIR,
                                const IR::P4Action *act, bool isDefaultAction);
    bool preorder(const IR::PathExpression *pe) override;
    bool isActionParameter(const IR::Expression *expression) const;
    void processMethod(const P4::ExternMethod *method) override;

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

class CRCChecksumAlgorithmPNA : public EBPF::CRCChecksumAlgorithm {
 public:
    CRCChecksumAlgorithmPNA(const EBPF::EBPFProgram *program, cstring name, int width)
        : EBPF::CRCChecksumAlgorithm(program, name, width) {}

    static void emitUpdateMethod(EBPF::CodeBuilder *builder, int crcWidth);
};

class CRC16ChecksumAlgorithmPNA : public CRCChecksumAlgorithmPNA {
 public:
    CRC16ChecksumAlgorithmPNA(const EBPF::EBPFProgram *program, cstring name)
        : CRCChecksumAlgorithmPNA(program, name, 16) {
        initialValue = "0"_cs;
        // We use a 0x8005 polynomial.
        // 0xA001 comes from 0x8005 value bits reflection.
        polynomial = "0xA001"_cs;
        updateMethod = "crc16_update"_cs;
        finalizeMethod = "crc16_finalize"_cs;
    }

    static void emitGlobals(EBPF::CodeBuilder *builder);
};

class CRC32ChecksumAlgorithmPNA : public CRCChecksumAlgorithmPNA {
 public:
    CRC32ChecksumAlgorithmPNA(const EBPF::EBPFProgram *program, cstring name)
        : CRCChecksumAlgorithmPNA(program, name, 32) {
        initialValue = "0xffffffff"_cs;
        // We use a 0x04C11DB7 polynomial.
        // 0xEDB88320 comes from 0x04C11DB7 value bits reflection.
        polynomial = "0xEDB88320"_cs;
        updateMethod = "crc32_update"_cs;
        finalizeMethod = "crc32_finalize"_cs;
    }

    static void emitGlobals(EBPF::CodeBuilder *builder);
};

class EBPFHashAlgorithmTypeFactoryPNA : public EBPF::EBPFHashAlgorithmTypeFactoryPSA {
 public:
    static EBPFHashAlgorithmTypeFactoryPNA *instance() {
        static EBPFHashAlgorithmTypeFactoryPNA factory;
        return &factory;
    }

    void emitGlobals(EBPF::CodeBuilder *builder) {
        CRC16ChecksumAlgorithmPNA::emitGlobals(builder);
        CRC32ChecksumAlgorithmPNA::emitGlobals(builder);
    }

    EBPF::EBPFHashAlgorithmPSA *create(int type, const EBPF::EBPFProgram *program, cstring name);
};

}  // namespace P4::TC

#endif /* BACKENDS_TC_EBPFCODEGEN_H_ */
