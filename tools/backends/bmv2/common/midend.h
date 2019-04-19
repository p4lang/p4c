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

#ifndef BACKENDS_BMV2_COMMON_MIDEND_H_
#define BACKENDS_BMV2_COMMON_MIDEND_H_

#include "ir/ir.h"
#include "lower.h"
#include "frontends/common/options.h"
#include "midend/convertEnums.h"

namespace BMV2 {

/**
This class implements a policy suitable for the ConvertEnums pass.
The policy is: convert all enums that are not part of the v1model.
Use 32-bit values for all enums.
*/
class EnumOn32Bits : public P4::ChooseEnumRepresentation {
    cstring filename;

    bool convert(const IR::Type_Enum* type) const override {
        if (type->srcInfo.isValid()) {
            auto sourceFile = type->srcInfo.getSourceFile();
            if (sourceFile.endsWith(filename))
                // Don't convert any of the standard enums
                return false;
        }
        return true;
    }
    unsigned enumSize(unsigned) const override
    { return 32; }

 public:
    /// Convert all enums except all the ones appearing in the
    /// specified file.
    explicit EnumOn32Bits(cstring filename) : filename(filename) { }
};

class MidEnd : public PassManager {
 public:
    // These will be accurate when the mid-end completes evaluation
    P4::ReferenceMap    refMap;
    P4::TypeMap         typeMap;
    const IR::ToplevelBlock   *toplevel = nullptr;
    P4::ConvertEnums::EnumMapping enumMap;
    bool isv1;

    explicit MidEnd(CompilerOptions& options) {
        isv1 = options.isv1();
        refMap.setIsV1(isv1);  // must be done BEFORE creating passes
    }
    const IR::ToplevelBlock* process(const IR::P4Program *&program) {
        program = program->apply(*this);
        return toplevel; }
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_MIDEND_H_ */
