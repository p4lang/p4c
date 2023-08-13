/*
Copyright 2019 Orange

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

#ifndef BACKENDS_UBPF_UBPFCONTROL_H_
#define BACKENDS_UBPF_UBPFCONTROL_H_

#include <map>
#include <set>
#include <vector>

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfControl.h"
#include "backends/ubpf/ubpfProgram.h"
#include "backends/ubpf/ubpfTable.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/map.h"
#include "ubpfRegister.h"

namespace UBPF {

class UBPFControlBodyTranslator : public EBPF::ControlBodyTranslator {
 public:
    const UBPFControl *control;
    std::set<const IR::Parameter *> toDereference;
    std::vector<cstring> saveAction;
    P4::P4CoreLibrary &p4lib;

    std::vector<UBPFRegister *> registersLookups;

    explicit UBPFControlBodyTranslator(const UBPFControl *control);
    void processMethod(const P4::ExternMethod *method) override;
    void processApply(const P4::ApplyMethod *method) override;
    void processFunction(const P4::ExternFunction *function) override;
    void processChecksumReplace2(const P4::ExternFunction *function);
    void processChecksumReplace4(const P4::ExternFunction *function);
    bool preorder(const IR::PathExpression *expression) override;
    bool preorder(const IR::MethodCallStatement *s) override;
    bool preorder(const IR::MethodCallExpression *expression) override;
    bool preorder(const IR::AssignmentStatement *a) override;
    bool preorder(const IR::BlockStatement *s) override;
    bool preorder(const IR::ExitStatement *) override;
    bool preorder(const IR::ReturnStatement *) override;
    bool preorder(const IR::IfStatement *statement) override;
    bool preorder(const IR::SwitchStatement *statement) override;
    bool preorder(const IR::Operation_Binary *b) override;
    bool comparison(const IR::Operation_Relation *b);
    bool preorder(const IR::Member *expression) override;
    cstring createHashKeyInstance(const P4::ExternFunction *function);
    void emitAssignmentStatement(const IR::AssignmentStatement *a);
    bool emitRegisterRead(const IR::AssignmentStatement *a, const IR::MethodCallExpression *method);
};

class UBPFControl : public EBPF::EBPFControl {
 public:
    const UBPFProgram *program;
    const IR::ControlBlock *controlBlock;
    const IR::Parameter *headers;
    const IR::Parameter *parserHeaders;
    // replace references to headers with references to parserHeaders
    cstring passVariable;
    UBPFControlBodyTranslator *codeGen;

    std::set<const IR::Parameter *> toDereference;
    std::map<cstring, UBPFTable *> tables;
    std::map<cstring, UBPFRegister *> registers;

    UBPFControl(const UBPFProgram *program, const IR::ControlBlock *block,
                const IR::Parameter *parserHeaders);

    void emit(EBPF::CodeBuilder *builder);
    void emitDeclaration(EBPF::CodeBuilder *builder, const IR::Declaration *decl);
    void emitTableTypes(EBPF::CodeBuilder *builder);
    void emitTableInstances(EBPF::CodeBuilder *builder);
    void emitTableInitializers(EBPF::CodeBuilder *builder);
    bool build();

    UBPFTable *getTable(cstring name) const {
        auto result = ::get(tables, name);
        BUG_CHECK(result != nullptr, "No table named %1%", name);
        return result;
    }

    UBPFRegister *getRegister(cstring name) const {
        auto result = ::get(registers, name);
        BUG_CHECK(result != nullptr, "No register named %1%", name);
        return result;
    }

 protected:
    void scanConstants();
};

}  // namespace UBPF

#endif /* BACKENDS_UBPF_UBPFCONTROL_H_ */
