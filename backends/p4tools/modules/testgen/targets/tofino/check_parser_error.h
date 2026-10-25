/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_CHECK_PARSER_ERROR_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_CHECK_PARSER_ERROR_H_

#include "ir/ir.h"

namespace P4::P4Tools::P4Testgen::Tofino {

/**
 * Walks a given IR node to detect whether it contains a reference to the Tofino parser error.
 * This is primarily used to detect that an ingress control references the parser error variable.
 * If that is the case, the semantics of parser errors change in the Tofino parser. It will not
 * drop packets and instead forward the packet to the MAU.
 */
class CheckParserError : public Inspector {
 private:
    bool parserErrorFound = false;

 public:
    CheckParserError() { setName("CheckParserError"); }

    bool hasParserError() const { return parserErrorFound; }

    void postorder(const IR::Member *member) override {
        if (member->member == "parser_err") {
            parserErrorFound = true;
        }
    }
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_CHECK_PARSER_ERROR_H_ */
