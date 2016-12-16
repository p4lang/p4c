/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _BACKENDS_EBPF_EBPFOBJECT_H_
#define _BACKENDS_EBPF_EBPFOBJECT_H_

#include "target.h"
#include "ebpfModel.h"
#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "codeGen.h"

namespace EBPF {

class EBPFProgram;
class EBPFParser;
class EBPFControl;
class EBPFTable;
class EBPFType;

cstring nameFromAnnotation(const IR::Annotations* annotations, cstring defaultValue);

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
    const IR::ToplevelBlock*  toplevel;
    P4::ReferenceMap*    refMap;
    P4::TypeMap*         typeMap;
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
    cstring arrayIndexType = "u32";

    // write program as C source code
    void emit(CodeBuilder *builder) override;
    bool build();  // return 'true' on success

    EBPFProgram(const IR::P4Program* program, P4::ReferenceMap* refMap,
                P4::TypeMap* typeMap, const IR::ToplevelBlock* toplevel) :
            program(program), toplevel(toplevel),
            refMap(refMap), typeMap(typeMap),
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
