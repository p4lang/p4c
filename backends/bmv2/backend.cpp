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

#include "backend.h"
#include "control.h"
#include "deparser.h"
#include "expression.h"
#include "globals.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/unusedDeclarations.h"
#include "midend/actionSynthesis.h"
#include "midend/orderArguments.h"
#include "midend/removeLeftSlices.h"

#include "lower.h"
#include "header.h"
#include "parser.h"
#include "JsonObjects.h"
#include "portableSwitch.h"
#include "simpleSwitch.h"

namespace BMV2 {

cstring Backend::jsonAssignment(const IR::Type* type, bool inParser) {
    if (!inParser && type->is<IR::Type_Varbits>())
        return "assign_VL";
    if (type->is<IR::Type_HeaderUnion>())
        return "assign_union";
    if (type->is<IR::Type_Header>())
        return "assign_header";
    if (auto ts = type->to<IR::Type_Stack>()) {
        auto et = ts->elementType;
        if (et->is<IR::Type_HeaderUnion>())
            return "assign_union_stack";
        else
            return "assign_header_stack";
    }
    if (inParser)
        // Unfortunately set can do some things that assign cannot,
        // e.g., handle lookahead on the RHS.
        return "set";
    else
        return "assign";
}


}  // namespace BMV2
