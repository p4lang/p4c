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

#ifndef _BACKENDS_EBPF_EBPFCONTROL_H_
#define _BACKENDS_EBPF_EBPFCONTROL_H_

#include "ebpfObject.h"
#include "ebpfTable.h"

namespace EBPF {

class EBPFControl : public EBPFObject {
 public:
    const EBPFProgram*      program;
    const IR::ControlBlock* controlBlock;
    const IR::Parameter*    headers;
    const IR::Parameter*    accept;
    cstring                 hitVariable;

    std::set<const IR::Parameter*> toDereference;
    std::map<cstring, EBPFTable*>  tables;
    std::map<cstring, EBPFCounterTable*>  counters;

    explicit EBPFControl(const EBPFProgram* program, const IR::ControlBlock* block);
    virtual ~EBPFControl() {}
    void emit(CodeBuilder* builder);
    void emitDeclaration(const IR::Declaration* decl, CodeBuilder *builder);
    void emitTables(CodeBuilder* builder);
    bool build();
    EBPFTable* getTable(cstring name) const {
        auto result = get(tables, name);
        BUG_CHECK(result != nullptr, "No table named %1%", name);
        return result; }
    EBPFCounterTable* getCounter(cstring name) const {
        auto result = get(counters, name);
        BUG_CHECK(result != nullptr, "No counter named %1%", name);
        return result; }
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_EBPFCONTROL_H_ */
