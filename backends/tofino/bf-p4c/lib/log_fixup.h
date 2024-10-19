/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
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
