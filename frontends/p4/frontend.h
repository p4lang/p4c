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

#ifndef _P4_FRONTEND_H_
#define _P4_FRONTEND_H_

#include "ir/ir.h"
#include "../common/options.h"

namespace P4 {

class FrontEnd {
    std::vector<DebugHook> hooks;
 public:
    FrontEnd() = default;
    explicit FrontEnd(DebugHook hook) { hooks.push_back(hook); }
    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
    const IR::P4Program* run(const CompilerOptions& options, const IR::P4Program* program,
                             bool skipSideEffectOrdering = false);
};

}  // namespace P4

#endif /* _P4_FRONTEND_H_ */
