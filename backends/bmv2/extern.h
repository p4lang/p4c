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

#ifndef _BACKENDS_BMV2_EXTERN_H_
#define _BACKENDS_BMV2_EXTERN_H_

#include "ir/ir.h"
#include "lib/json.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "backend.h"
#include "helpers.h"
#include "JsonObjects.h"

namespace BMV2 {

class Extern : public Inspector {
    Backend*             backend;
    BMV2::JsonObjects*   json;

 protected:
    Util::JsonArray*
        addExternAttributes(const IR::Declaration_Instance* di, const IR::ExternBlock* block);

 public:
    const bool emitExterns;

    bool preorder(const IR::PackageBlock* b) override;
    bool preorder(const IR::Declaration_Instance* decl) override;

    explicit Extern(Backend *backend, const bool& emitExterns_) : backend(backend),
        json(backend->json), emitExterns(emitExterns_) { setName("Extern"); }
};

class ConvertExterns final : public PassManager {
 public:
    explicit ConvertExterns(Backend *b, const bool& emitExterns_) {
       passes.push_back(new Extern(b, emitExterns_));
       setName("ConvertExterns");
    }
};

}  // namespace BMV2

#endif  /* _BACKENDS_BMV2_EXTERN_H_ */
