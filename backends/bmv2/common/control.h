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

#ifndef BACKENDS_BMV2_COMMON_CONTROL_H_
#define BACKENDS_BMV2_COMMON_CONTROL_H_

#include "ir/ir.h"
#include "lib/json.h"
#include "controlFlowGraph.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "midend/convertEnums.h"
#include "expression.h"
#include "helpers.h"
#include "sharedActionSelectorCheck.h"

namespace BMV2 {

class ControlConverter : public Inspector {
    ConversionContext* ctxt;
    cstring            name;
    P4::P4CoreLibrary& corelib;

 protected:
    Util::IJson* convertTable(const CFG::TableNode* node,
                              Util::JsonArray* action_profiles,
                              BMV2::SharedActionSelectorCheck& selector_check);
    void convertTableEntries(const IR::P4Table *table, Util::JsonObject *jsonTable);
    cstring getKeyMatchType(const IR::KeyElement *ke);
    /// Return 'true' if the table is 'simple'
    bool handleTableImplementation(const IR::Property* implementation, const IR::Key* key,
                                   Util::JsonObject* table, Util::JsonArray* action_profiles,
                                   BMV2::SharedActionSelectorCheck& selector_check);
    Util::IJson* convertIf(const CFG::IfNode* node, cstring prefix);

 public:
    const bool emitExterns;
    bool preorder(const IR::P4Control* b) override;
    explicit ControlConverter(ConversionContext* ctxt, cstring name, const bool& emitExterns_) :
        ctxt(ctxt), name(name), corelib(P4::P4CoreLibrary::instance), emitExterns(emitExterns_)
    { setName("ControlConverter"); }
};

}  // namespace BMV2

#endif  /* BACKENDS_BMV2_COMMON_CONTROL_H_ */
