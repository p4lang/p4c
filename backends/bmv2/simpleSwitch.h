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

#ifndef _BACKENDS_BMV2_SIMPLESWITCH_H_
#define _BACKENDS_BMV2_SIMPLESWITCH_H_

#include <algorithm>
#include <cstring>
#include <set>
#include "frontends/p4/fromv1.0/v1model.h"
#include "backends/bmv2/backend.h"

namespace P4V1 {

void convertExternObjects(Util::JsonArray *result, BMV2::Backend *bmv2,
                          const P4::ExternMethod *em,
                          const IR::MethodCallExpression *mc,
                          const IR::StatOrDecl *s);

void convertExternFunctions(Util::JsonArray *result, BMV2::Backend *bmv2,
                            const P4::ExternFunction *ef,
                            const IR::MethodCallExpression *mc,
                            const IR::StatOrDecl* s);

void convertExternInstances(BMV2::Backend *backend,
                            const IR::Declaration *c,
                            const IR::ExternBlock* eb,
                            Util::JsonArray* action_profiles,
                            BMV2::SharedActionSelectorCheck& selector_check);

}  // namespace P4V1

#endif /* _BACKENDS_BMV2_SIMPLESWITCH_H_ */
