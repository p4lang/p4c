/*
Copyright 2017 VMware, Inc.

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

#ifndef MIDEND_NOMATCH_H_
#define MIDEND_NOMATCH_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace P4 {

/// Convert
/// state s { transition select (e) { ... } }
/// into
/// state s { transition select (e) { ... default: noMatch; }}
/// state noMatch { verify(false, error.NoMatch); transition reject; }
class HandleNoMatch : public Transform {
    MinimalNameGenerator nameGen;

 public:
    const IR::ParserState *noMatch = nullptr;
    HandleNoMatch() { setName("HandleNoMatch"); }
    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = Transform::init_apply(node);
        node->apply(nameGen);

        return rv;
    }
    const IR::Node *postorder(IR::SelectExpression *expression) override;
    const IR::Node *postorder(IR::P4Parser *parser) override;
    const IR::Node *postorder(IR::P4Program *program) override;
};

}  // namespace P4

#endif /* MIDEND_NOMATCH_H_ */
