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

#ifndef BACKENDS_BMV2_COMMON_HEADER_H_
#define BACKENDS_BMV2_COMMON_HEADER_H_

#include <list>
#include "ir/ir.h"
#include "lib/json.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "helpers.h"
#include "JsonObjects.h"
#include "programStructure.h"
#include "backends/bmv2/common/options.h"

namespace BMV2 {

class Backend;

class HeaderConverter : public Inspector {
    ConversionContext*   ctxt;
    cstring              scalarsName;
    cstring              scalarsTypeName;
    std::set<cstring>    visitedHeaders;

    const unsigned       boolWidth = 1;    // convert booleans to 1-bit integers
    const unsigned       errorWidth = 32;  // convert errors to 32-bit integers
    unsigned             scalars_width = 0;

 protected:
    Util::JsonArray* pushNewArray(Util::JsonArray* parent);
    void addHeaderType(const IR::Type_StructLike* st);
    void addHeaderField(const cstring& header, const cstring& name, int size, bool is_signed);
    Util::JsonArray* addHeaderUnionFields(cstring hdrName, const IR::Type_HeaderUnion* type);

 public:
    void addTypesAndInstances(const IR::Type_StructLike* type, bool meta);
    void addHeaderStacks(const IR::Type_Struct* type);
    bool isHeaders(const IR::Type_StructLike* st);

    Visitor::profile_t init_apply(const IR::Node* node) override;
    void end_apply(const IR::Node* node) override;

    bool preorder(const IR::Parameter* param) override;

    HeaderConverter(ConversionContext* ctxt, cstring scalarsName);
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_HEADER_H_ */
