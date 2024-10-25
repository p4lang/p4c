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

#ifndef _BACKENDS_TOFINO_BF_P4C_LOGGING_RESOURCES_PARSER_H_
#define _BACKENDS_TOFINO_BF_P4C_LOGGING_RESOURCES_PARSER_H_

/* clang-format off */
#include "ir/ir.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "resources_schema.h"
/* clang-format on */

using Logging::Resources_Schema_Logger;

namespace BFN {

/**
 *  \brief Class for generating JSON info with parser resources usage
 *  for visualization in P4I
 */
class ParserResourcesLogging : public ParserInspector {
 public:
    using ElementUsage = Resources_Schema_Logger::ElementUsage;
    using ParserResources = Resources_Schema_Logger::ParserResources;
    using ParserResourceUsage = Resources_Schema_Logger::ParserResourceUsage;
    using Phase0ResourceUsage = Resources_Schema_Logger::Phase0ResourceUsage;

    using ParserStateTransition = Resources_Schema_Logger::ParserStateTransitionResourceUsage;

    using ClotExtracts = ParserStateTransition::Clot_Extracts;
    using StateExtracts = ParserStateTransition::Extracts;
    using StateMatchesOn = ParserStateTransition::MatchesOn;
    using StateSavesTo = ParserStateTransition::SavesTo;

 private:
    struct ParserLogData {
        ParserResourceUsage *usage = nullptr;
        Phase0ResourceUsage *phase0 = nullptr;
    };

    bool collected = false;
    ParserResources *parserLogger = nullptr;

    std::map<cstring, ParserLogData> parsers;

    std::map<cstring, int> stateIds;
    std::map<const IR::BFN::LoweredParserMatch *, int> tcamIds[2];
    int nextTcamId[2] = {0};  // Next TCAM ID value array

    /// A parser match is a parser state.
    bool preorder(const IR::BFN::LoweredParserState *state) override;
    bool preorder(const IR::BFN::LoweredParser *parser) override;
    void end_apply() override { collected = true; }

    std::vector<ParserStateTransition *> logStateTransitionsByMatch(
        const std::string &nextStateName, const IR::BFN::LoweredParserState *prevState,
        const IR::BFN::LoweredParserMatch *match);

    int getTcamId(const IR::BFN::LoweredParserMatch *match, gress_t gress);

    int getStateId(const std::string &state);

    void logStateExtracts(const IR::BFN::LoweredParserMatch *match,
                          std::vector<StateExtracts *> &result);

    void logStateMatches(const IR::BFN::LoweredParserState *prevState,
                         const IR::BFN::LoweredParserMatch *match,
                         std::vector<StateMatchesOn *> &result);

    void logStateSaves(const IR::BFN::LoweredParserMatch *match,
                       std::vector<StateSavesTo *> &result);

 public:
    ParserResourcesLogging() {
        const int tcamRowsInit = Device::pardeSpec().numTcamRows() - 1;
        nextTcamId[0] = tcamRowsInit;
        nextTcamId[1] = tcamRowsInit;
    }

    const ParserResources *getLogger();
};

}  // namespace BFN

#endif /* _BACKENDS_TOFINO_BF_P4C_LOGGING_RESOURCES_PARSER_H_ */
