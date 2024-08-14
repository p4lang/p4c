/*
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

#ifndef FRONTENDS_P4_VALIDATESTRINGANNOTATIONS_H_
#define FRONTENDS_P4_VALIDATESTRINGANNOTATIONS_H_

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/error.h"

/// @file
/// @brief Check validity of built-in string annotations.

namespace P4 {

/// Checks that the build-in string annotations (\@name, \@deprecated, \@noWarn) have string
/// arguments.
class ValidateStringAnnotations final : public Inspector {
    TypeMap *typeMap;

 public:
    explicit ValidateStringAnnotations() {}

    void postorder(const IR::Annotation *annotation) override {
        const auto name = annotation->name;
        if (name != IR::Annotation::nameAnnotation &&
            name != IR::Annotation::deprecatedAnnotation &&
            name != IR::Annotation::noWarnAnnotation) {
            return;
        }
        if (annotation->expr.size() != 1)
            ::error(ErrorType::ERR_INVALID, "%1%: annotation must have exactly 1 argument",
                    annotation);
        auto e0 = annotation->expr.at(0);
        if (!e0->is<IR::StringLiteral>())
            ::error(ErrorType::ERR_TYPE_ERROR, "%1%: @%2% annotation's value must be a string", e0,
                    annotation->name.originalName);
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_VALIDATESTRINGANNOTATIONS_H_  */
