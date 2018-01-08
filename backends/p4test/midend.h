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

#ifndef _BACKENDS_P4TEST_MIDEND_H_
#define _BACKENDS_P4TEST_MIDEND_H_

#include "ir/ir.h"
#include "frontends/common/options.h"
#include "frontends/p4/evaluator/evaluator.h"

namespace P4Test {

class MidEnd : public PassManager {
 public:
    P4::ReferenceMap    refMap;
    P4::TypeMap         typeMap;
    IR::ToplevelBlock   *toplevel = nullptr;

    explicit MidEnd(CompilerOptions& options);
    IR::ToplevelBlock* process(const IR::P4Program *&program) {
        program = program->apply(*this);
        return toplevel; }
};

}   // namespace P4Test

#endif /* _BACKENDS_P4TEST_MIDEND_H_ */
