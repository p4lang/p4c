/*
Copyright 2017 VMware, Inc.

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

#ifndef _IR_DECLARATION_H_
#define _IR_DECLARATION_H_

#include "node.h"

namespace IR {

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
};

}  // namespace IR

#endif  /* _IR_DECLARATION_H_ */
