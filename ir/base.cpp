// Copyright 2013-present Barefoot Networks, Inc.
// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"

namespace P4::IR {

cstring Annotation::getName() const {
    BUG_CHECK(name == IR::Annotation::nameAnnotation, "%1%: Only works on name annotations", this);
    if (needsParsing())
        // This can happen if this method is invoked before we have parsed
        // annotation bodies.
        return name;
    return getSingleString();
}

cstring Annotation::getSingleString(bool error) const {
    // getExpr() only works when the annotation body was successfully parsed into a plain
    // (unstructured) expression list. Annotations written using the structured syntax
    // (@foo[...] or @foo[k=v]) or whose body failed to parse do not carry an expression
    // list at all, so calling getExpr() on them would trigger an internal BUG. Since this
    // helper is reached from many places (getName(), externalName(), controlPlaneName(),
    // and warning suppression checks) that run throughout the frontend -- including before
    // annotation bodies are fully validated -- it must handle these cases gracefully rather
    // than assuming annotationKind() == Kind::Unstructured.
    switch (annotationKind()) {
        case Kind::Unparsed:
            // The annotation body has not been parsed yet, or failed to parse; in the
            // latter case an error has already been reported elsewhere, so avoid a
            // duplicate diagnostic here.
            return cstring::empty;
        case Kind::StructuredKVList:
        case Kind::StructuredExpressionList:
            if (error)
                ::P4::error(ErrorType::ERR_INVALID,
                            "%1%: annotation cannot be written using structured annotation syntax",
                            this);
            return cstring::empty;
        case Kind::Unstructured:
            break;
    }
    const auto &expr = getExpr();
    if (expr.size() != 1) {
        if (error) ::P4::error(ErrorType::ERR_INVALID, "%1%: should contain a string", this);
        return cstring::empty;
    }
    auto str = expr[0]->to<IR::StringLiteral>();
    if (str == nullptr) {
        if (error) ::P4::error(ErrorType::ERR_INVALID, "%1%: should contain a string", this);
        return cstring::empty;
    }
    return str->value;
}

cstring IDeclaration::externalName(cstring replace /* = cstring() */) const {
    if (const auto *annotated = to<IAnnotated>()) {
        if (const auto *anno = annotated->getAnnotation(IR::Annotation::nameAnnotation))
            return anno->getName();
        if (replace) return replace;
    }

    return getName().toString();
}

cstring IDeclaration::controlPlaneName(cstring replace /* = cstring() */) const {
    cstring name = externalName(replace);
    return name.startsWith(".") ? name.substr(1) : name;
}

}  // namespace P4::IR
