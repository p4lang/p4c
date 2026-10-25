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

#ifndef BACKENDS_TOFINO_BF_P4C_SPECS_MATCH_REGISTER_SPEC_H_
#define BACKENDS_TOFINO_BF_P4C_SPECS_MATCH_REGISTER_SPEC_H_

#include <iostream>

#include "lib/cstring.h"

namespace P4 {

class MatchRegisterSpec {
 public:
    MatchRegisterSpec();
    explicit MatchRegisterSpec(cstring);

    cstring toString() const;

    bool operator==(const MatchRegisterSpec &other) const { return name == other.name; }

    cstring name;
    size_t size;
    int id;

    static int s_id;

    bool operator<(const MatchRegisterSpec &other) const {
        if (size < other.size) return true;
        if (other.size < size) return false;
        if (id < other.id) return true;
        if (other.id > id) return false;
        return false;
    }

    friend std::ostream &operator<<(std::ostream &out, const MatchRegisterSpec &c);
};

inline std::ostream &operator<<(std::ostream &out, const MatchRegisterSpec &c) {
    return out << c.name;
}

}  // namespace P4

#endif /* BACKENDS_TOFINO_BF_P4C_SPECS_MATCH_REGISTER_SPEC_H_ */
