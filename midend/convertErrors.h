/* Copyright 2021 Intel Corporation

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

#ifndef _MIDEND_CONVERTERRORS_H_
#define _MIDEND_CONVERTERRORS_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Policy function: Determines if the error type should be converted to Ser_Enum.
 * This Serializable Enum can later be eliminated by replacing the constant by
 * their respective values.
 */
class ChooseErrorRepresentation {
 public:
    virtual ~ChooseErrorRepresentation() {}
    // If true this type has to be converted.
    virtual bool convert(const IR::Type_Error* /* type */) const { return false; };
    // errorCount is the number of different error values.
    // The returned value is the width of Type_Bits used
    // to represent the errors.  Obviously, we must have
    // 2^(return) >= errorCount.
    virtual unsigned errorSize(unsigned /* errorCount */) const { return 0; };
    // This function allows backends to override the values for the error constants.
    // Default values for error constants is a sequence of numbers starting with 0.
    virtual IR::IndexedVector<IR::SerEnumMember>* assignValues(IR::Type_Error* type,
                                                               unsigned width) const {
        auto members = new IR::IndexedVector<IR::SerEnumMember>;
        unsigned idx = 0;
        for (auto d : type->members) {
            members->push_back(new IR::SerEnumMember(
                d->name.name, new IR::Constant(IR::Type_Bits::get(width), idx++)));
        }
        return members;
    }
};

/** implement a pass to convert Type_Error to Type_SerEnum
 *
 *  User must provide a class to extend ChooseErrorRepresentation
 *  to specify the width of the generated Type_Bits. User must
 *  also implement a policy to decide whether an Error type
 *  shall be converted.
 *
 *  Example:
 *  error {
 *      ....
 *  }
 *  if the policy is to convert error to serializable enum with underlying type to bit<16>,
 *  then the above is replaced by
 *
 *  enum bit<width> error {
 *      ....
 *  }
 *
 *  @pre none
 *  @post all Type_Error types accepted by 'policy' are converted.
 */
class DoConvertErrors : public Transform {
    friend class ConvertErrors;

    ChooseErrorRepresentation* policy;

 public:
    explicit DoConvertErrors(ChooseErrorRepresentation* policy) : policy(policy) {
        CHECK_NULL(policy);
        setName("DoConvertErrors");
    }

    const IR::Node* preorder(IR::Type_Error* type) {
        bool convert = policy->convert(type);
        if (!convert) return type;
        IR::IndexedVector<IR::SerEnumMember> members;
        unsigned count = type->members.size();
        unsigned width = policy->errorSize(count);
        return new IR::Type_SerEnum("error", IR::Type_Bits::get(width),
                                    *policy->assignValues(type, width));
    }
};

class ConvertErrors : public PassManager {
    DoConvertErrors* convertErrors{nullptr};

 public:
    ConvertErrors(ReferenceMap* refMap, TypeMap* typeMap, ChooseErrorRepresentation* policy,
                  TypeChecking* typeChecking = nullptr)
        : convertErrors(new DoConvertErrors(policy)) {
        if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(convertErrors);
        passes.push_back(new ClearTypeMap(typeMap));
        setName("ConvertErrors");
    }
};

}  // namespace P4

#endif /* _MIDEND_CONVERTERRORS_H_ */
