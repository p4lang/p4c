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

#ifndef _BACKENDS_BMV2_MIDEND_H_
#define _BACKENDS_BMV2_MIDEND_H_

#include "ir/ir.h"
#include "frontends/common/options.h"
#include "midend/convertEnums.h"
#include "helpers.h"
#include "options.h"

namespace BMV2 {

class MidEnd : public PassManager {
 public:
    // These will be accurate when the mid-end completes evaluation
    P4::ReferenceMap    refMap;
    P4::TypeMap         typeMap;
    const IR::ToplevelBlock   *toplevel = nullptr;
    P4::ConvertEnums::EnumMapping enumMap;

    explicit MidEnd(BMV2Options& options);
    const IR::ToplevelBlock* process(const IR::P4Program *&program) {
        program = program->apply(*this);
        return toplevel; }
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_MIDEND_H_ */
