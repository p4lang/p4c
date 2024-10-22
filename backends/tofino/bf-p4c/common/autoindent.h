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

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_AUTOINDENT_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_AUTOINDENT_H_

#include "lib/log.h"

/// A RAII helper that indents when it's created and unindents by the same
/// amount when it's destroyed.
/// TODO: This should live in indent.h.
struct AutoIndent {
    explicit AutoIndent(indent_t &indent, int indentBy = 1) : indent(indent), indentBy(indentBy) {
        indent += indentBy;
    }
    ~AutoIndent() { indent -= indentBy; }

 private:
    indent_t &indent;
    int indentBy;
};

#endif /* BACKENDS_TOFINO_BF_P4C_COMMON_AUTOINDENT_H_ */
