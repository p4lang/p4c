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

#ifndef _BACKENDS_BMV2_ACTION_H_
#define _BACKENDS_BMV2_ACTION_H_

#include "ir/ir.h"
#include "backend.h"
#include "simpleSwitch.h"

namespace BMV2 {

class ConvertActions : public Inspector {
    Backend*               backend;
    P4::ReferenceMap*      refMap;
    P4::TypeMap*           typeMap;
    BMV2::JsonObjects*     json;
    ExpressionConverter*   conv;

    void convertActionBody(const IR::Vector<IR::StatOrDecl>* body,
                           Util::JsonArray* result);
    void convertActionParams(const IR::ParameterList *parameters,
                             Util::JsonArray* params);
    void createActions();
    bool preorder(const IR::PackageBlock* package);
 public:
    const bool emitExterns;
    explicit ConvertActions(Backend *backend, const bool& emitExterns_) : backend(backend),
    refMap(backend->getRefMap()), typeMap(backend->getTypeMap()),
    json(backend->json), conv(backend->getExpressionConverter()), emitExterns(emitExterns_)
    { setName("ConvertActions"); }
};

}  // namespace BMV2

#endif  /* _BACKENDS_BMV2_ACTION_H_ */
