/*
Copyright 2018 VMware, Inc.

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

#ifndef _FRONTENDS_P4_CHECKNAMEDARGS_H_
#define _FRONTENDS_P4_CHECKNAMEDARGS_H_

#include "ir/ir.h"

namespace P4 {

/// A method call must have either all or none of the arguments named.
/// We also check that no argument appears twice.
/// We also check that no optional parameter has a default value.
class CheckNamedArgs : public Inspector {
 public:
    CheckNamedArgs() {
        setName("CheckNamedArgs");
    }

    bool checkArguments(const IR::Vector<IR::Argument> *arguments);
    bool preorder(const IR::MethodCallExpression* call) override
    { return checkArguments(call->arguments); }
    bool preorder(const IR::Declaration_Instance* call) override
    { return checkArguments(call->arguments); }
    bool preorder(const IR::Parameter* parameter) override;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_CHECKNAMEDARGS_H_ */
