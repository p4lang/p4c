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

#ifndef BACKENDS_P4TEST_MIDEND_H_
#define BACKENDS_P4TEST_MIDEND_H_

#include "frontends/common/options.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "ir/ir.h"

namespace P4Test {

class MidEnd : public PassManager {
    std::vector<DebugHook> hooks;

 public:
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    IR::ToplevelBlock *toplevel = nullptr;

    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
    explicit MidEnd(CompilerOptions &options, std::ostream *outStream = nullptr);
    IR::ToplevelBlock *process(const IR::P4Program *&program) {
        addDebugHooks(hooks, true);
        program = program->apply(*this);
        return toplevel;
    }
};

}  // namespace P4Test

#endif /* BACKENDS_P4TEST_MIDEND_H_ */
