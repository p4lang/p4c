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

#pragma once
#include <string>

namespace BFASM {
// Singleton class representing the assembler version
class Version {
 public:
    static const std::string getVersion() {
        static Version v;
        return std::to_string(v.major) + "." + std::to_string(v.minor) + "." +
               std::to_string(v.patch);
    }

 private:
    static constexpr int major = 1;
    static constexpr int minor = 0;
    static constexpr int patch = 1;

    Version() {}

 public:
    // disable any other constructors
    Version(Version const &) = delete;
    void operator=(Version const &) = delete;
};

}  // namespace BFASM
