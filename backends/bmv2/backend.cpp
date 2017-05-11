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

#include "action.h"
#include "backend.h"
#include "control.h"
#include "copyAnnotations.h"
#include "deparser.h"
#include "errorcode.h"
#include "expression.h"
#include "extern.h"
#include "frontends/p4/methodInstance.h"
#include "header.h"
#include "parser.h"
#include "JsonObjects.h"

namespace BMV2 {

void Backend::process(const IR::ToplevelBlock* tb) {
    setName("BackEnd");
    addPasses({
        new DiscoverStructure(&structure),
        new ErrorCodesVisitor(&errorCodesMap),
    });
    tb->getProgram()->apply(*this);
}

/// BMV2 Backend that takes the top level block and converts it to a JsonObject.
void Backend::convert(const IR::ToplevelBlock* tb, CompilerOptions& options) {
    toplevel.emplace("program", options.file);
    toplevel.emplace("__meta__", json->meta);
    toplevel.emplace("header_types", json->header_types);
    toplevel.emplace("headers", json->headers);
    toplevel.emplace("header_stacks", json->header_stacks);
    field_lists = mkArrayField(&toplevel, "field_lists");
    toplevel.emplace("errors", json->errors);
    toplevel.emplace("enums", json->enums);
    toplevel.emplace("parsers", json->parsers);
    toplevel.emplace("deparsers", json->deparsers);
    meter_arrays = mkArrayField(&toplevel, "meter_arrays");
    counters = mkArrayField(&toplevel, "counter_arrays");
    register_arrays = mkArrayField(&toplevel, "register_arrays");
    calculations = mkArrayField(&toplevel, "calculations");
    learn_lists = mkArrayField(&toplevel, "learn_lists");
    toplevel.emplace("actions", json->actions);
    toplevel.emplace("pipelines", json->pipelines);
    checksums = mkArrayField(&toplevel, "checksums");
    force_arith = mkArrayField(&toplevel, "force_arith");
    toplevel.emplace("extern_instances", json->externs);
    toplevel.emplace("field_aliases", json->field_aliases);

    /// generate error types
    for (const auto &p : errorCodesMap) {
        auto name = p.first->toString();
        auto type = p.second;
        json->add_error(name, type);
    }

    // This visitor is used in multiple passes to convert expression to json
    conv = new ExpressionConverter(this);

    // if (psa) tlb->apply(new ConvertExterns());

    PassManager codegen_passes = {
        new CopyAnnotations(refMap, &blockTypeMap),
        new ConvertHeaders(this),
        new ConvertExterns(this),  // only run when mode == PSA
        new ConvertParser(this),
        new ConvertActions(this),
        new ConvertControl(this),
        new ConvertDeparser(this),
    };
    codegen_passes.setName("CodeGen");
    tb->getMain()->apply(codegen_passes);
}

}  // namespace BMV2
