/*
    Copyright 2018 MNK Consulting, LLC.
    http://mnkcg.com

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

#include "flattenHeaders.h"

#include <ostream>
#include <string>
#include <vector>

#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/map.h"
#include "midend/flattenInterfaceStructs.h"

namespace P4 {

void FindHeaderTypesToReplace::createReplacement(const IR::Type_Header *type,
                                                 AnnotationSelectionPolicy *policy) {
    if (replacement.count(type->name)) return;
    replacement.emplace(type->name,
                        new StructTypeReplacement<IR::Type_StructLike>(typeMap, type, policy));
}

bool FindHeaderTypesToReplace::preorder(const IR::Type_Header *type) {
    // check if header has structs in it and create flat replacement if it does
    for (auto f : type->fields) {
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_Struct>()) {
            createReplacement(type, policy);
            break;
        }
    }
    return false;
}

/////////////////////////////////

const IR::Node *ReplaceHeaders::preorder(IR::P4Program *program) {
    if (findHeaderTypesToReplace->empty()) {
        // nothing to do
        prune();
    }
    return program;
}

const IR::Node *ReplaceHeaders::postorder(IR::Type_Header *type) {
    auto canon = typeMap->getTypeType(getOriginal(), true);
    auto name = canon->to<IR::Type_Header>()->name;
    auto repl = findHeaderTypesToReplace->getReplacement(name);
    if (repl != nullptr) {
        LOG3("Replace " << type << " with " << repl->replacementType);
        BUG_CHECK(repl->replacementType->is<IR::Type_Header>(), "%1% not a header", type);
        return repl->replacementType;
    }
    return type;
}

const IR::Node *ReplaceHeaders::postorder(IR::Member *expression) {
    // Find out if this applies to one of the parameters that are being replaced.
    const IR::Expression *e = expression;
    cstring prefix = "";
    const IR::Type_Header *h = nullptr;
    while (auto mem = e->to<IR::Member>()) {
        e = mem->expr;
        prefix = cstring(".") + mem->member + prefix;
        auto type = typeMap->getType(e, true);
        if ((h = type->to<IR::Type_Header>())) break;
    }
    if (h == nullptr) return expression;

    auto repl = findHeaderTypesToReplace->getReplacement(h->name);
    if (repl == nullptr) {
        return expression;
    }
    // At this point we know that e is an expression of the form
    // param.field1.etc.hdr, where hdr needs to be replaced.
    auto newFieldName = ::get(repl->fieldNameRemap, prefix);
    const IR::Expression *result;
    if (newFieldName.isNullOrEmpty()) {
        auto type = typeMap->getType(getOriginal(), true);
        // This could be, for example, a method like setValid.
        if (!type->is<IR::Type_Struct>()) return expression;
        if (getParent<IR::Member>() != nullptr)
            // We only want to process the outermost Member
            return expression;
        if (isWrite()) {
            ::error(ErrorType::ERR_UNSUPPORTED,
                    "%1%: writing to a structure nested in a header is not supported", expression);
            return expression;
        }
        result = repl->explode(e, prefix);
    } else {
        result = new IR::Member(e, newFieldName);
    }
    LOG3("Replacing " << expression << " with " << result);
    return result;
}

}  // namespace P4
