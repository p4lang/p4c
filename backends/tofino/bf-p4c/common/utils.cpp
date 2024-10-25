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

#include "utils.h"

#include <regex>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/device.h"

bool ghost_only_on_other_pipes(int pipe_id) {
    if (pipe_id < 0) return false;  // invalid pipe id

    auto options = BackendOptions();
    if (!Device::hasGhostThread()) return false;
    if (options.ghost_pipes == 0) return false;

    if ((options.ghost_pipes & (0x1 << pipe_id)) == 0) {
        return true;
    }
    return false;
}

std::tuple<bool, cstring, int, int> get_key_slice_info(const cstring &input) {
    std::string s(input);
    std::smatch match;
    std::regex sliceRegex(R"(\[([0-9]+):([0-9]+)\])");
    std::regex_search(s, match, sliceRegex);
    if (match.size() == 3) {
        int hi = std::atoi(match[1].str().c_str());
        int lo = std::atoi(match[2].str().c_str());
        return std::make_tuple(true, match.prefix().str(), hi, lo);
    }
    return std::make_tuple(false, ""_cs, -1, -1);
}

std::pair<cstring, cstring> get_key_and_mask(const cstring &input) {
    std::string k(input);
    std::smatch match;
    std::regex maskRegex(R"([\s]*&[\s]*(0x[a-fA-F0-9]+))");
    std::regex_search(k, match, maskRegex);
    cstring key = input;
    cstring mask = ""_cs;
    if (match.size() >= 2) {
        key = match.prefix().str();
        mask = match[1].str();
    }
    return std::make_pair(key, mask);
}

const IR::Vector<IR::Expression> *getListExprComponents(const IR::Node &node) {
    const IR::Vector<IR::Expression> *components;

    if (node.is<IR::StructExpression>()) {
        auto freshVec = new IR::Vector<IR::Expression>();
        // sadly no .reserve() on IR::Vector
        for (auto named : node.to<IR::StructExpression>()->components) {
            freshVec->push_back(named->expression);
        }
        components = freshVec;
    } else if (node.is<IR::ListExpression>()) {
        const IR::ListExpression *sourceListExpr = node.to<IR::ListExpression>();
        components = &(sourceListExpr->components);
    } else {
        BUG("getListExprComponents called with a non-list-like expression %s", IR::dbp(&node));
    }

    return components;
}
void end_fatal_error() {
#if BAREFOOT_INTERNAL
    if (auto res = BFNContext::get().errorReporter().verify_checks();
        res == BfErrorReporter::CheckResult::SUCCESS) {
        std::clog << "An expected fatal error was raised, exiting.\n";
        std::exit(0);
    } else if (res == BfErrorReporter::CheckResult::FAILURE) {
        std::clog << "A diagnostic check failed in fatal error processing\n";
    }
#endif /* BAREFOOT_INTERNAL */
    throw Util::CompilationError("Compilation failed!");
}

bool is_starter_pistol_table(const cstring &tableName) {
    return (tableName.startsWith("$") && tableName.endsWith("_starter_pistol"));
}
