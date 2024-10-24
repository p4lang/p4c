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

#include "match_register.h"

#include <sstream>

#include "ir/ir.h"
#include "ir/json_generator.h"
#include "ir/json_loader.h"

int MatchRegister::s_id = 0;

cstring MatchRegister::toString() const {
    std::stringstream tmp;
    tmp << *this;
    return tmp.str();
}

MatchRegister::MatchRegister() : name(""), size(0), id(0) {}

MatchRegister::MatchRegister(cstring n) : name(n), id(s_id++) {
    if (name.find("byte"))
        size = 1;
    else if (name.find("half"))
        size = 2;
    else
        BUG("Invalid parser match register %s", name);
}

void MatchRegister::toJSON(JSONGenerator &json) const { json << *this; }

/* static */
MatchRegister MatchRegister::fromJSON(JSONLoader &json) {
    if (auto *v = json.json->to<JsonString>()) return MatchRegister(cstring(v->c_str()));
    BUG("Couldn't decode JSON value to parser match register");
    return MatchRegister();
}

P4::JSONGenerator &operator<<(P4::JSONGenerator &out, const MatchRegister &c) {
    return out << c.toString();
}
