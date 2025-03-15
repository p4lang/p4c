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

#ifndef BACKENDS_TOFINO_BF_ASM_PARSER_H_
#define BACKENDS_TOFINO_BF_ASM_PARSER_H_

#include "asm-types.h"
#include "backends/tofino/bf-asm/json.h"
#include "backends/tofino/bf-asm/target.h"
#include "sections.h"
#include "vector.h"

/**
 * @brief Base class of Tofino parser in assembler
 *
 * For Tofino 1/2, the class Parser is derived.
 */
class BaseParser : virtual public Configurable {
 protected:
    int lineno = -1;
};

/**
 * @brief Base class of parser assembly section
 */
class BaseAsmParser : public Section {
 public:
    explicit BaseAsmParser(const char *name_) : Section(name_) {}
};

#endif /* BACKENDS_TOFINO_BF_ASM_PARSER_H_ */
