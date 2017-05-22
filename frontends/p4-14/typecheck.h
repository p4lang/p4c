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

#ifndef _FRONTENDS_P4_14_TYPECHECK_H_
#define _FRONTENDS_P4_14_TYPECHECK_H_

#include "ir/ir.h"

/* This is the P4 v1.0/v1.1 typechecker/type inference algorithm */
class TypeCheck : public PassManager {
    std::map<const IR::Node *, const IR::Type *>        actionArgUseTypes;
    int                                                 iterCounter = 0;
    class AssignInitialTypes;
    class InferExpressionsBottomUp;
    class InferExpressionsTopDown;
    class InferActionArgsBottomUp;
    class InferActionArgsTopDown;
    class AssignActionArgTypes;
    class MakeImplicitCastsExplicit;
 public:
    TypeCheck();
    const IR::Node *apply_visitor(const IR::Node *, const char *) override;
};

#endif /* _FRONTENDS_P4_14_TYPECHECK_H_ */
