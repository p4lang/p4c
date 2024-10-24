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

#ifndef BACKENDS_TOFINO_BF_P4C_LIB_LOG_FIXUP_H_
#define BACKENDS_TOFINO_BF_P4C_LIB_LOG_FIXUP_H_

#include "lib/log.h"

class logfix {
    std::string text;
    friend std::ostream &operator<<(std::ostream &out, const logfix &lf) {
        bool newline = false;
        for (char ch : lf.text) {
            if (newline) out << Log::endl;
            if (!(newline = ch == '\n')) out << ch;
        }
        return out;
    }

 public:
    explicit logfix(std::string s) : text(s) {}
    explicit logfix(const std::stringstream &ss) : text(ss.str()) {}
};

#endif /* BACKENDS_TOFINO_BF_P4C_LIB_LOG_FIXUP_H_ */
