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

#ifndef _MIDEND_NOMATCH_H_
#define _MIDEND_NOMATCH_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

/// Convert
/// state s { transition select (e) { ... } }
/// into
/// state s { transition select (e) { ... default: noMatch; }}
/// state noMatch { verify(false, error.NoMatch); transition reject; }
class DoHandleNoMatch : public Transform {
    const IR::ParserState* noMatch = nullptr;
    NameGenerator* nameGen;
 public:
    explicit DoHandleNoMatch(NameGenerator* ng): nameGen(ng)
    { CHECK_NULL(ng); setName("DoHandleNoMatch"); }
    const IR::Node* postorder(IR::SelectExpression* expression) override;
    const IR::Node* preorder(IR::P4Parser* parser) override;
};

class HandleNoMatch : public PassManager {
 public:
    explicit HandleNoMatch(ReferenceMap* refMap) {
        passes.push_back(new ResolveReferences(refMap));
        passes.push_back(new DoHandleNoMatch(refMap));
        setName("HandleNoMatch");
    }
};

}  // namespace P4

#endif /* _MIDEND_NOMATCH_H_ */
