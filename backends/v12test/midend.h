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

#ifndef _BACKENDS_V12TEST_MIDEND_H_
#define _BACKENDS_V12TEST_MIDEND_H_

#include "ir/ir.h"
#include "frontends/common/options.h"
#include "frontends/p4/evaluator/evaluator.h"

namespace V12Test {

class MidEnd {
    std::vector<DebugHook> hooks;
 public:
    MidEnd() = default;
    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
    IR::ToplevelBlock* process(CompilerOptions& options, const IR::P4Program* program);
};

}   // namespace V12Test

#endif /* _BACKENDS_V12TEST_MIDEND_H_ */
