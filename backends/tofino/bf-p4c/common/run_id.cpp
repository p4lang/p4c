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

#include "bf-p4c/common/run_id.h"

#include <iomanip>
#include <sstream>

#include <boost/uuid/random_generator.hpp>

RunId::RunId() {
    auto uuid = boost::uuids::random_generator()();
    auto uuidBytes = std::vector<uint8_t>(uuid.begin(), uuid.end());
    std::stringstream ss;
    ss << std::hex;
    for (uint8_t b : uuidBytes) {
        ss << std::setw(2) << std::setfill('0') << (int)b;
    }
    _runId = ss.str();
    // Take only the first 16 characters to keep backward-compatibility with previous
    // implementations.
    _runId.resize(16);
}
