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

#ifndef BACKENDS_TOFINO_BF_ASM_ESCAPE_H_
#define BACKENDS_TOFINO_BF_ASM_ESCAPE_H_

#include <iomanip>
#include <iostream>

#include "lib/hex.h"

class escape {
    std::string str;

 public:
    explicit escape(const std::string &s) : str(s) {}
    friend std::ostream &operator<<(std::ostream &os, escape e);
};

inline std::ostream &operator<<(std::ostream &os, escape e) {
    for (char ch : e.str) {
        switch (ch) {
            case '\n':
                os << "\\n";
                break;
            case '\t':
                os << "\\t";
                break;
            case '\\':
                os << "\\\\";
                break;
            default:
                if (ch < 32 || ch >= 127)
                    os << "\\x" << hex(ch & 0xff, 2, '0');
                else
                    os << ch;
        }
    }
    return os;
}

#endif /* BACKENDS_TOFINO_BF_ASM_ESCAPE_H_ */
