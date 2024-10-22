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

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_FRONT_END_POLICY_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_FRONT_END_POLICY_H_

#include "frontends/p4/frontend.h"

namespace BFN {

struct FrontEndPolicy : P4::FrontEndPolicy {
    P4::ParseAnnotations *getParseAnnotations() const override { return pars; }

    bool skipSideEffectOrdering() const override { return skip_side_effect_ordering; }

    explicit FrontEndPolicy(P4::ParseAnnotations *parseAnnotations)
        : pars(parseAnnotations), skip_side_effect_ordering(false) {}

    explicit FrontEndPolicy(P4::ParseAnnotations *parseAnnotations, bool skip_side_effect_ordering)
        : pars(parseAnnotations), skip_side_effect_ordering(skip_side_effect_ordering) {}

 private:
    P4::ParseAnnotations *pars;
    bool skip_side_effect_ordering;
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_COMMON_FRONT_END_POLICY_H_ */
