#ifndef _BACKENDS_EBPF_EBPFOBJECT_H_
#define _BACKENDS_EBPF_EBPFOBJECT_H_

#include "target.h"
#include "ebpfModel.h"
#include "ir/ir.h"
#include "frontends/common/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "codeGen.h"

namespace EBPF {

class EBPFProgram;
class EBPFParser;
class EBPFControl;
class EBPFTable;
class EBPFType;

// Base class for EBPF objects
class EBPFObject {
 public:
	virtual ~EBPFObject() {}
    virtual void emit(CodeBuilder* builder) = 0;
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> const T* to() const {
        return dynamic_cast<const T*>(this); }
    template<typename T> T* to() {
        return dynamic_cast<T*>(this); }
};

class EBPFProgram : public EBPFObject {
 public:
    const IR::P4Program* program;
    const P4::BlockMap*  blockMap;
    P4::ReferenceMap*    refMap;
    const P4::TypeMap*   typeMap;
    EBPFParser*          parser;
    EBPFControl*         control;
    EBPFModel           &model;

    cstring endLabel;
    cstring offsetVar;
    cstring zeroKey;
    cstring functionName;
    cstring errorVar;
    cstring errorEnum;
    cstring license = "GPL";  // TODO: this should be a compiler option probably
    cstring arrayIndexType = "size_t";

    // write program as C source code
    void emit(CodeBuilder *builder) override;
    bool build();  // return 'true' on success

    EBPFProgram(const IR::P4Program* program, const P4::BlockMap* blockMap) :
            program(program), blockMap(blockMap),
            refMap(blockMap->refMap), typeMap(blockMap->typeMap),
			parser(nullptr), control(nullptr), model(EBPFModel::instance) {
        offsetVar = EBPFModel::reserved("packetOffsetInBits");
        zeroKey = EBPFModel::reserved("zero");
        functionName = EBPFModel::reserved("filter");
        errorVar = EBPFModel::reserved("errorCode");
        endLabel = EBPFModel::reserved("end");
        errorEnum = EBPFModel::reserved("errorCodes");
    }

 private:
    void emitIncludes(CodeBuilder* builder);
    void emitPreamble(CodeBuilder* builder);
    void emitTypes(CodeBuilder* builder);
    void emitTables(CodeBuilder* builder);
    void emitHeaderInstances(CodeBuilder* builder);
    void emitIninitailizeHeaders(CodeBuilder* builder);
    void createLocalVariables(CodeBuilder* builder);
    void emitPipeline(CodeBuilder* builder);
    void emitLicense(CodeBuilder* builder);
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_EBPFOBJECT_H_ */
