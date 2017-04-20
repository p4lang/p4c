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

#ifndef _P4_INFER_ARCHITECTURE_H_
#define _P4_INFER_ARCHITECTURE_H_

#include "ir/ir.h"
#include "ir/visitor.h"
#include "typeMap.h"
#include "common/model.h"

namespace P4 {

class InferArchitecture : public Inspector {
 private:
    TypeMap *typeMap;
    V2Model& v2model;
 public:
    InferArchitecture(TypeMap *typeMap)
        : typeMap(typeMap), v2model(P4::V2Model::instance) {
    }
 public:
    bool preorder(const IR::Type_Control *node) override;
    bool preorder(const IR::Type_Parser *node) override;
    bool preorder(const IR::Type_Extern *node) override;
    bool preorder(const IR::Type_Package *node) override;
    bool preorder(const IR::P4Program* program) override;
    bool preorder(const IR::Declaration_MatchKind* kind) override;
};

} // namespace P4

#endif
