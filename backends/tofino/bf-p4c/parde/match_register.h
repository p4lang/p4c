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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_MATCH_REGISTER_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_MATCH_REGISTER_H_

#include <iostream>

#include "backends/tofino/bf-p4c/specs/match_register_spec.h"
#include "lib/cstring.h"

namespace P4 {

class JSONGenerator;
class JSONLoader;

class MatchRegister : public MatchRegisterSpec {
 public:
    MatchRegister() = default;
    explicit MatchRegister(cstring n) : MatchRegisterSpec(n) {}
    explicit MatchRegister(const MatchRegisterSpec &spec) : MatchRegisterSpec(spec) {}

    /// JSON serialization/deserialization.
    void toJSON(JSONGenerator &json) const;
    static MatchRegister fromJSON(JSONLoader &json);
};

}  // namespace P4

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_MATCH_REGISTER_H_ */
