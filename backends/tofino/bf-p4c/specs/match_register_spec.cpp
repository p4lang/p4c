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

#include "match_register_spec.h"

#include "lib/exceptions.h"

namespace P4 {
int MatchRegisterSpec::s_id = 0;

cstring MatchRegisterSpec::toString() const {
    std::stringstream tmp;
    tmp << *this;
    return tmp.str();
}

MatchRegisterSpec::MatchRegisterSpec() : name(""), size(0), id(0) {}

MatchRegisterSpec::MatchRegisterSpec(cstring n) : name(n), id(s_id++) {
    if (name.find("byte"))
        size = 1;
    else if (name.find("half"))
        size = 2;
    else
        BUG("Invalid parser match register %s", name);
}

}  // namespace P4
