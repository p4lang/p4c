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

#ifndef BACKENDS_BMV2_COMMON_DEPARSER_H_
#define BACKENDS_BMV2_COMMON_DEPARSER_H_

#include "ir/ir.h"
#include "lib/json.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "expression.h"
#include "backend.h"

namespace BMV2 {

class DeparserConverter : public Inspector {
    ConversionContext*     ctxt;
    P4::P4CoreLibrary&     corelib;

 protected:
    Util::IJson* convertDeparser(const IR::P4Control* ctrl);
    void convertDeparserBody(const IR::Vector<IR::StatOrDecl>* body, Util::JsonArray* result);
 public:
    bool preorder(const IR::P4Control* ctrl);

    explicit DeparserConverter(ConversionContext* ctxt) :
        ctxt(ctxt), corelib(P4::P4CoreLibrary::instance) {
        setName("DeparserConverter"); }
};

}  // namespace BMV2

#endif  /* BACKENDS_BMV2_COMMON_DEPARSER_H_ */
