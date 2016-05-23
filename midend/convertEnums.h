/*
Copyright 2016 VMware, Inc.

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

#ifndef _MIDEND_CONVERTENUMS_H_
#define _MIDEND_CONVERTENUMS_H_

#include "ir/ir.h"
#include "frontends/common/typeMap.h"

namespace P4 {

// Policy function: given a number of enum values should return
// the size of a Type_Bits type used to represent the values.
class ChooseEnumRepresentation {
 public:
    virtual ~ChooseEnumRepresentation() {}
    // If true this type has to be converted.
    virtual bool convert(const IR::Type_Enum* type) const = 0;
    // enumCount is the number of different enum values.
    // The returned value is the width of Type_Bits used
    // to represent the enum.  Obviously, we must have
    // 2^(return) >= enumCount.
    virtual unsigned enumSize(unsigned enumCount) const = 0;
};

class EnumRepresentation {
    std::map<cstring, unsigned> repr;
 public:
    const IR::Type_Bits* type;

    EnumRepresentation(Util::SourceInfo srcInfo, unsigned width)
    { type = new IR::Type_Bits(srcInfo, width, false); }
    void add(cstring decl)
    { repr.emplace(decl, repr.size()); }
    unsigned get(cstring decl) const
    { return ::get(repr, decl); }
};

class ConvertEnums : public Transform {
    std::map<const IR::Type_Enum*, EnumRepresentation*> repr;
    ChooseEnumRepresentation* policy;
    TypeMap* typeMap;
 public:
    ConvertEnums(ChooseEnumRepresentation* policy, TypeMap* typeMap)
            : policy(policy), typeMap(typeMap)
    { CHECK_NULL(policy); CHECK_NULL(typeMap); }
    const IR::Node* preorder(IR::Type_Enum* type) override;
    const IR::Node* postorder(IR::Type_Name* type) override;
    const IR::Node* postorder(IR::Member* expression) override;
};

}  // namespace P4

#endif /* _MIDEND_CONVERTENUMS_H_ */
