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

#ifndef _BACKENDS_TOFINO_BF_P4C_LOGGING_RESOURCES_CLOT_H_
#define _BACKENDS_TOFINO_BF_P4C_LOGGING_RESOURCES_CLOT_H_

/* clang-format off */
#include <map>
#include <vector>
#include "ir/ir.h"

#include "resources_schema.h"
/* clang-format on */

using Logging::Resources_Schema_Logger;
class ClotInfo;  // Forward declaration

namespace BFN {

class ClotResourcesLogging : public ParserInspector {
 public:
    using ClotResourceUsage = Resources_Schema_Logger::ClotResourceUsage;
    using ClotUsage = Resources_Schema_Logger::ClotUsage;
    using ClotField = Resources_Schema_Logger::ClotField;

 protected:
    const ClotInfo &clotInfo;
    ClotResourceUsage *clotLogger = nullptr;
    std::vector<ClotResourceUsage *> clotUsages = std::vector<ClotResourceUsage *>(2);
    bool collected = false;
    std::map<unsigned, unsigned> clotTagToChecksumUnit;
    std::vector<std::map<unsigned, std::vector<ClotUsage *>>> usageData;

    bool usingClots() const;

    std::vector<ClotUsage *> &getUsageData(gress_t gress, unsigned tag);

    bool preorder(const IR::BFN::LoweredParserState *state);
    void end_apply() override;

    void collectClotUsages(const IR::BFN::LoweredParserMatch *match,
                           const IR::BFN::LoweredParserState *state, gress_t gress);

    void collectExtractClotInfo(const IR::BFN::LoweredExtractClot *extract,
                                const IR::BFN::LoweredParserState *state, gress_t gress);

    void logClotUsages();

    ClotUsage *logExtractClotInfo(cstring parser_state, bool hasChecksum, int length, int offset,
                                  unsigned tag, const Clot *clot);

 public:
    std::vector<ClotResourceUsage *> getLoggers();

    explicit ClotResourcesLogging(const ClotInfo &clotInfo);
};

}  // namespace BFN

#endif /* _BACKENDS_TOFINO_BF_P4C_LOGGING_RESOURCES_CLOT_H_ */
