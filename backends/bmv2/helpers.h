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

#ifndef _BACKENDS_BMV2_HELPERS_H_
#define _BACKENDS_BMV2_HELPERS_H_

#include "ir/ir.h"
#include "lib/json.h"
#include "analyzer.h"
#include "frontends/common/model.h"

namespace BMV2 {

Util::IJson* nodeName(const CFG::Node* node);
Util::JsonArray* mkArrayField(Util::JsonObject* parent, cstring name);
Util::JsonArray* mkParameters(Util::JsonObject* object);
Util::JsonArray* pushNewArray(Util::JsonArray* parent);
Util::JsonObject* mkPrimitive(cstring name, Util::JsonArray* appendTo);
cstring extVisibleName(const IR::IDeclaration* decl);
cstring stringRepr(mpz_class value, unsigned bytes = 0);
unsigned nextId(cstring group);

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_HELPERS_H_ */
