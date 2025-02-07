/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sections.h>

#include <string>

namespace BFASM {

// Singleton class representing the assembler flexible_headers
class FlexibleHeaders : public Section {
 private:
    std::unique_ptr<json::vector> flexHeaders;

    FlexibleHeaders() : Section("flexible_headers") {}

    void input(VECTOR(value_t) args, value_t data) {
        if (!CHECKTYPE(data, tVEC)) return;
        flexHeaders = std::move(toJson(data.vec));
    }

    void output(json::map &ctxtJson) {
        if (flexHeaders != nullptr) ctxtJson["flexible_headers"] = std::move(flexHeaders);
    }

 public:
    // disable any other constructors
    FlexibleHeaders(FlexibleHeaders const &) = delete;
    void operator=(FlexibleHeaders const &) = delete;

    static FlexibleHeaders singleton_flexHeaders;
} FlexibleHeaders::singleton_flexHeaders;

};  // namespace BFASM
