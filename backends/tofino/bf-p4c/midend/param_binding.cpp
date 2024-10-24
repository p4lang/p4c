/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "param_binding.h"

#include "frontends/p4/typeMap.h"

void ParamBinding::bind(const IR::Parameter *param) {
    auto *type = typeMap->getType(param);
    if (!type->is<IR::Type_StructLike>()) {
        return;
    }
    if (!by_type.count(type)) {
        LOG1("adding param binding for " << param->name << ": " << type->toString());
        if (isV1)
            by_type.emplace(type, new IR::V1InstanceRef(param->name, type, param->annotations));
        else
            by_type.emplace(type, new IR::InstanceRef(param->name, type, param->annotations));
    }
    LOG2("adding param binding for node " << param->id << ": " << type->toString() << ' '
                                          << param->name);
    by_param[param] = by_type.at(type);
}

void ParamBinding::bind(const IR::Declaration_Variable *var) {
    if (by_declvar[var]) return;
    LOG2("creating instance for var " << var->name);
    by_declvar[var] = new IR::InstanceRef(var->name, typeMap->getType(var), var->annotations);
}

void ParamBinding::postorder(const IR::Parameter *param) { bind(param); }

void ParamBinding::postorder(const IR::Declaration_Variable *var) { bind(var); }
