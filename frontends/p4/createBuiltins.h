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

#ifndef _P4_CREATEBUILTINS_H_
#define _P4_CREATEBUILTINS_H_

#include "ir/ir.h"

/*
 * Creates accept and reject states.
 * Adds parentheses to action invocations in tables:
    e.g., actions = { a; } becomes actions = { a(); }
 * Parser states without selects will transition to reject.
 */
namespace P4 {

class CreateBuiltinsPolicy {
public:
    virtual ~CreateBuiltinsPolicy() {}
    /**
       If the policy returns true the parser is processed,
       otherwise it is left unchanged.
    */
    virtual bool convert(const IR::P4Parser* parser) const = 0;
};

class CreateBuiltins final : public Modifier {
    bool addNoAction;
    CreateBuiltinsPolicy* policy;
 public:
    using Modifier::postorder;
    CreateBuiltins(CreateBuiltinsPolicy* policy = nullptr)
            : policy(policy) { setName("CreateBuiltins"); }
    void postorder(IR::ParserState* state) override;
    void postorder(IR::P4Parser* parser) override;
    void postorder(IR::ActionListElement* element) override;
    void postorder(IR::ExpressionValue* property) override;
    bool preorder(IR::P4Table* table) override;
    void postorder(IR::ActionList* actions) override;
    void postorder(IR::TableProperties* properties) override;
};
}  // namespace P4

#endif /* _P4_CREATEBUILTINS_H_ */
