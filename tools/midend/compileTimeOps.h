/*
Copyright 2016 VMware, Inc.

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

#ifndef _MIDEND_COMPILETIMEOPS_H_
#define _MIDEND_COMPILETIMEOPS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Check that operations which are only defined at compile-time
 * do not exist in the program.
 *
 * @pre This should be run after inlining, constant folding and strength reduction.
 * @post There are no IR::Mod and IR::Div operations in the program.
 */
class CompileTimeOperations : public Inspector {
 public:
    CompileTimeOperations() { setName("CompileTimeOperations"); }
    void err(const IR::Node* expression)
    { ::error("%1%: could not evaluate at compilation time", expression); }
    void postorder(const IR::Mod* expression) override
    { err(expression); }
    void postorder(const IR::Div* expression) override
    { err(expression); }
};

}  // namespace P4

#endif /* _MIDEND_COMPILETIMEOPS_H_ */
