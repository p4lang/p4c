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

#ifndef BACKENDS_BMV2_COMMON_PARSER_H_
#define BACKENDS_BMV2_COMMON_PARSER_H_

#include "ir/ir.h"
#include "lib/json.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "expression.h"
#include "helpers.h"

namespace BMV2 {

class JsonObjects;

class ParserConverter : public Inspector {
    ConversionContext*   ctxt;
    cstring              name;
    P4::P4CoreLibrary&   corelib;

 protected:
    void convertSimpleKey(const IR::Expression* keySet, mpz_class& value, mpz_class& mask) const;
    unsigned combine(const IR::Expression* keySet, const IR::ListExpression* select,
                     mpz_class& value, mpz_class& mask, bool& is_vset, cstring& vset_name) const;
    Util::IJson* stateName(IR::ID state);
    Util::IJson* convertParserStatement(const IR::StatOrDecl* stat);
    Util::IJson* convertSelectKey(const IR::SelectExpression* expr);
    Util::IJson* convertPathExpression(const IR::PathExpression* expr);
    Util::IJson* createDefaultTransition();
    cstring jsonAssignment(const IR::Type* type, bool inParser);
    std::vector<Util::IJson*> convertSelectExpression(const IR::SelectExpression* expr);

 public:
    bool preorder(const IR::P4Parser* p) override;
    explicit ParserConverter(ConversionContext* ctxt, cstring name = "parser")
        : ctxt(ctxt), name(name), corelib(P4::P4CoreLibrary::instance) {
        setName("ParserConverter");
    }
};

}  // namespace BMV2

#endif  /* BACKENDS_BMV2_COMMON_PARSER_H_ */
