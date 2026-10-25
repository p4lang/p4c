/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IR_DECLARATION_H_
#define IR_DECLARATION_H_

#include "ir/id.h"
#include "ir/node.h"

namespace P4::IR {

/// The Declaration interface, representing objects with names.
class IDeclaration : public virtual INode {
 public:
    /// @return the name of the declared object.
    virtual ID getName() const = 0;

    /**
     * @return the user-visible name of the object, derived from its '@name'
     * annotation if available.
     * @param replace  If provided, a fallback name which is used if the object
     *                 doesn't have an explicit '@name' annotation.
     */
    virtual cstring externalName(cstring replace = cstring()) const;

    /**
     * @return the name by which the object should be referred to from the
     * control plane. This is usually the same as externalName(), but any
     * leading "." (which indicates that the name is global) is stripped out.
     *
     * @param replace  If provided, a fallback name which is used if the object
     *                 doesn't have an explicit '@name' annotation.
     */
    cstring controlPlaneName(cstring replace = cstring()) const;

    virtual ~IDeclaration() {}

    DECLARE_TYPEINFO_WITH_TYPEID(IDeclaration, NodeKind::IDeclaration, INode);
};

}  // namespace P4::IR

#endif /* IR_DECLARATION_H_ */
