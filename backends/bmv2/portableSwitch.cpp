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

#include "frontends/common/model.h"
#include "portableSwitch.h"

namespace P4 {

const IR::P4Program* PsaProgramStructure::create(const IR::P4Program* program) {
    createTypes();
    createHeaders();
    createExterns();
    createParsers();
    createActions();
    createControls();
    createDeparsers();
    return program;
}

void PsaProgramStructure::createTypes() {
    // add header_types to json
    // e.g. use existing API in JsonObjects.h
    // json->add_header_type();

    // add header_union_types to json
    // add errors to json
    // add enums to json
}

void PsaProgramStructure::createHeaders() {
    // add headers to json
    // add header_stacks to json
    // add header_unions to json
}

void PsaProgramStructure::createParsers() {
    // add parsers to json
}

void PsaProgramStructure::createExterns() {
    // add parse_vsets to json
    // add meter_arrays to json
    // add counter_arrays to json
    // add register_arrays to json
    // add checksums to json
    // add learn_list to json
    // add calculations to json
    // add extern_instances to json
}

void PsaProgramStructure::createActions() {
    // add actions to json
}

void PsaProgramStructure::createControls() {
    // add pipelines to json
}

void PsaProgramStructure::createDeparsers() {
    // add deparsers to json
}

void InspectPsaProgram::postorder(const IR::P4Parser* p) {
    // inspect IR::P4Parser
    // populate structure->parsers
}

void InspectPsaProgram::postorder(const IR::P4Control* c) {
    // inspect IR::P4Control
    // populate structure->pipelines or
    //          structure->deparsers
}

void InspectPsaProgram::postorder(const IR::Type_Header* h) {
    // inspect IR::Type_Header
    // populate structure->header_types;
}

void InspectPsaProgram::postorder(const IR::Type_HeaderUnion* hu) {
    // inspect IR::Type_HeaderUnion
    // populate structure->header_union_types;
}

void InspectPsaProgram::postorder(const IR::Declaration_Variable* var) {
    // inspect IR::Declaration_Variable
    // populate structure->headers or
    //          structure->header_stacks or
    //          structure->header_unions
    // based on the type of the variable
}

void InspectPsaProgram::postorder(const IR::Declaration_Instance* di) {
    // inspect IR::Declaration_Instance,
    // populate structure->meter_arrays or
    //          structure->counter_arrays or
    //          structure->register_arrays or
    //          structure->extern_instances or
    //          structure->checksums
    // based on the type of the instance
}

void InspectPsaProgram::postorder(const IR::P4Action* act) {
    // inspect IR::P4Action,
    // populate structure->actions
}

void InspectPsaProgram::postorder(const IR::Type_Error* err) {
    // inspect IR::Type_Error
    // populate structure->errors.
}

}  // namespace P4
