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

#include "ir/json_generator.h"
#include "ir/json_loader.h"
#include "ir/json_parser.h"
#include "lib/exceptions.h"

namespace P4 {

void MatchRegister::toJSON(JSONGenerator &json) const { json.emit(toString()); }

/* static */
MatchRegister MatchRegister::fromJSON(JSONLoader &json) {
    if (json.is<JsonString>()) {
        return MatchRegister(json.as<JsonString>());
    }
    BUG("Couldn't decode JSON value to parser match register");
    return {};
}

}  // namespace P4
