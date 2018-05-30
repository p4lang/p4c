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

#ifndef _BACKENDS_BMV2_DEPARSER_H_
#define _BACKENDS_BMV2_DEPARSER_H_

#include "ir/ir.h"
#include "lib/json.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "expression.h"
#include "backend.h"

namespace BMV2 {

class DeparserConverter : public Inspector {
    Backend*               backend;
    P4::ReferenceMap*      refMap;
    P4::TypeMap*           typeMap;
    BMV2::JsonObjects*     json;
    ExpressionConverter*   conv;

 protected:
    Util::IJson* convertDeparser(const IR::P4Control* ctrl);
    void convertDeparserBody(const IR::Vector<IR::StatOrDecl>* body, Util::JsonArray* result);
 public:
    bool preorder(const IR::P4Control* ctrl);

    explicit DeparserConverter(Backend* backend) :
        backend(backend), refMap(backend->getRefMap()),
        typeMap(backend->getTypeMap()),
        json(backend->json),
        conv(backend->getExpressionConverter()){ setName("DeparserConverter"); }
};

}  // namespace BMV2

#endif  /* _BACKENDS_BMV2_DEPARSER_H_ */
