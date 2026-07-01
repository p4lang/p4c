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
 public:
    explicit ValidateStringAnnotations() = default;

    void postorder(const IR::Annotation *annotation) override {
        const auto name = annotation->name;
        if (name != IR::Annotation::nameAnnotation &&
            name != IR::Annotation::deprecatedAnnotation &&
            name != IR::Annotation::noWarnAnnotation) {
            return;
        }

        // These built-in annotations must be plain, unstructured annotations of the form
        // @name("..."), i.e. a single expression argument that was successfully parsed.
        // Other kinds are rejected here:
        //  - Kind::Unparsed means the annotation body could not be parsed as a single
        //    expression (e.g. it has zero or more than one argument, or invalid syntax);
        //    an error has already been reported for it, so we simply avoid a duplicate
        //    diagnostic.
        //  - Kind::StructuredKVList and Kind::StructuredExpressionList correspond to the
        //    structured annotation syntax (@name[...]), which is not supported for these
        //    built-in annotations. In particular, a key-value list does not carry an
        //    expression list at all, so calling getExpr() on it would trigger an internal
        //    BUG; we must check the annotation kind before calling getExpr().
        switch (annotation->annotationKind()) {
            case IR::Annotation::Kind::Unparsed:
                return;
            case IR::Annotation::Kind::StructuredKVList:
            case IR::Annotation::Kind::StructuredExpressionList:
                error(ErrorType::ERR_INVALID,
                      "%1%: @%2% annotation cannot be written using structured annotation syntax",
                      annotation, annotation->name.originalName);
                return;
            case IR::Annotation::Kind::Unstructured:
                break;
        }

        const auto &expr = annotation->getExpr();
        if (expr.size() != 1) {
            error(ErrorType::ERR_INVALID, "%1%: annotation must have exactly 1 argument",
                  annotation);
            return;
        }
        const auto *e0 = expr.at(0);
        if (!e0->is<IR::StringLiteral>()) {
            error(ErrorType::ERR_TYPE_ERROR, "%1%: @%2% annotation's value must be a string", e0,
                  annotation->name.originalName);
        }
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_VALIDATESTRINGANNOTATIONS_H_  */
