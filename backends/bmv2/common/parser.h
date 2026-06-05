/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_COMMON_PARSER_H_
#define BACKENDS_BMV2_COMMON_PARSER_H_

#include "expression.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "helpers.h"
#include "ir/ir.h"
#include "lib/json.h"

namespace P4::BMV2 {

class JsonObjects;

class ParserConverter : public Inspector {
    ConversionContext *ctxt;
    cstring name;
    P4::P4CoreLibrary &corelib;

 protected:
    void convertSimpleKey(const IR::Expression *keySet, big_int &value, big_int &mask) const;
    unsigned combine(const IR::Expression *keySet, const IR::ListExpression *select, big_int &value,
                     big_int &mask, bool &is_vset, cstring &vset_name) const;
    Util::IJson *stateName(IR::ID state);
    Util::IJson *convertParserStatement(const IR::StatOrDecl *stat);
    Util::IJson *convertSelectKey(const IR::SelectExpression *expr);
    Util::IJson *convertPathExpression(const IR::PathExpression *expr);
    Util::IJson *createDefaultTransition();
    cstring jsonAssignment(const IR::Type *type);
    std::vector<Util::IJson *> convertSelectExpression(const IR::SelectExpression *expr);
    void addValueSets(const IR::P4Parser *parser);

 public:
    bool preorder(const IR::P4Parser *p) override;
    explicit ParserConverter(ConversionContext *ctxt, cstring name = "parser"_cs)
        : ctxt(ctxt), name(name), corelib(P4::P4CoreLibrary::instance()) {
        setName("ParserConverter");
    }
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_COMMON_PARSER_H_ */
