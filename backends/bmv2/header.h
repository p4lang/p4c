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

#ifndef _BACKENDS_BMV2_CONVERTHEADERS_H_
#define _BACKENDS_BMV2_CONVERTHEADERS_H_

#include <list>
#include "ir/ir.h"
#include "lib/json.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "helpers.h"

namespace BMV2 {

class ConvertHeaders : public Inspector {
    BMV2::Backend*    backend;
    std::set<const IR::Type_StructLike*> headerTypesCreated;
    std::set<const IR::Type*> headerInstancesCreated;

 protected:
    Util::JsonArray* pushNewArray(Util::JsonArray* parent);
    void pushFields(const IR::Type_StructLike *st, Util::JsonArray *fields);
    void createJsonType(const IR::Type_StructLike* st);

 public:
    void createHeaderTypeAndInstance(const IR::Type_StructLike* st, bool meta);
    void createStack(const IR::Type_Stack *stack, bool meta);
    void createNestedStruct(const IR::Type_StructLike *st, bool meta);
    bool hasStructLikeMember(const IR::Type_StructLike *st, bool meta);

    bool preorder(const IR::PackageBlock* b) override;
    bool preorder(const IR::Type_Extern* e) override;
    bool preorder(const IR::Type_Parser* e) override;
    bool preorder(const IR::Type_Control* ctrl) override;
    bool preorder(const IR::Parameter* param) override;

    explicit ConvertHeaders(Backend* backend): backend(backend)
    {}
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_CONVERTHEADERS_H_ */
