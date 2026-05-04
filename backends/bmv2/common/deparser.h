/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_COMMON_DEPARSER_H_
#define BACKENDS_BMV2_COMMON_DEPARSER_H_

#include "backend.h"
#include "expression.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/json.h"

namespace P4::BMV2 {

class DeparserConverter : public Inspector {
    ConversionContext *ctxt;
    cstring name;
    P4::P4CoreLibrary &corelib;

 protected:
    Util::IJson *convertDeparser(const IR::P4Control *ctrl);
    void convertDeparserBody(const IR::Vector<IR::StatOrDecl> *body, Util::JsonArray *order,
                             Util::JsonArray *primitives);

 public:
    bool preorder(const IR::P4Control *ctrl) override;

    explicit DeparserConverter(ConversionContext *ctxt, cstring name = "deparser"_cs)
        : ctxt(ctxt), name(name), corelib(P4::P4CoreLibrary::instance()) {
        setName("DeparserConverter");
    }
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_COMMON_DEPARSER_H_ */
