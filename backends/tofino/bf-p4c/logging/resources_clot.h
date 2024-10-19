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
