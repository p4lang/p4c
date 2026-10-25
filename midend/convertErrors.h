/*
 * Copyright 2021 Intel Corporation
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_CONVERTERRORS_H_
#define MIDEND_CONVERTERRORS_H_

#include <map>

#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/null.h"
#include "lib/safe_vector.h"
#include "midend/convertEnums.h"

namespace P4 {

/**
 * Policy function: given a number of error values should return
 * the size of a Type_Bits type used to represent the values.
 * This class is lefted from enum conversion path.
 */
class ChooseErrorRepresentation {
 public:
    virtual ~ChooseErrorRepresentation() = default;

    /// If true this type has to be converted.
    virtual bool convert(const IR::Type_Error *type) const = 0;

    /// errorCount is the number of different error values.
    /// The returned value is the width of Type_Bits used
    /// to represent the error.  Obviously, we must have
    /// 2^(return) >= errorCount.
    virtual unsigned errorSize(unsigned errorCount) const = 0;

    /// This function allows backends to override the values for the error constants.
    /// Default values for error constants is a sequence of numbers starting with 0.
    virtual IR::IndexedVector<IR::SerEnumMember> *assignValues(IR::Type_Error *type,
                                                               unsigned width) const;
};

class DoConvertErrors : public Transform {
    friend class ConvertErrors;

    std::map<cstring, P4::EnumRepresentation *> repr;
    ChooseErrorRepresentation *policy;
    P4::TypeMap *typeMap;

 public:
    DoConvertErrors(ChooseErrorRepresentation *policy, P4::TypeMap *typeMap)
        : policy(policy), typeMap(typeMap) {
        CHECK_NULL(policy);
        CHECK_NULL(typeMap);
        setName("DoConvertErrors");
    }
    const IR::Node *preorder(IR::Type_Error *type) override;
    const IR::Node *postorder(IR::Type_Name *type) override;
    const IR::Node *postorder(IR::Member *member) override;
};

class ConvertErrors : public PassManager {
    DoConvertErrors *convertErrors{nullptr};

 public:
    using ErrorMapping = decltype(DoConvertErrors::repr);
    ConvertErrors(P4::TypeMap *typeMap, ChooseErrorRepresentation *policy,
                  P4::TypeChecking *typeChecking = nullptr)
        : convertErrors(new DoConvertErrors(policy, typeMap)) {
        if (typeChecking == nullptr) {
            typeChecking = new P4::TypeChecking(nullptr, typeMap);
        }
        passes.push_back(typeChecking);
        passes.push_back(convertErrors);
        passes.push_back(new P4::ClearTypeMap(typeMap));
        setName("ConvertErrors");
    }

    ErrorMapping getErrorMapping() const { return convertErrors->repr; }
};
}  // namespace P4

#endif /* MIDEND_CONVERTERRORS_H_ */
