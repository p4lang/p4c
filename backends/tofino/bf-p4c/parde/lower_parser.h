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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_LOWER_PARSER_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_LOWER_PARSER_H_

#include "bf-p4c/parde/parser_header_sequences.h"
#include "common/utils.h"
#include "ir/ir.h"
#include "logging/pass_manager.h"
#include "logging/phv_logging.h"

class ClotInfo;
class PhvInfo;
class FieldDefUse;

/**
 * \defgroup LowerParser LowerParser
 * \ingroup parde
 *
 * \brief Replace field-based parser and deparser IR with container-based parser
 * and deparser IR.
 *
 * Replace the high-level parser and deparser IRs, which operate on fields and
 * field-like objects, into the low-level IRs which operate on PHV containers
 * and constants.
 *
 * This is a lossy transformation; no information about fields remains in the
 * lowered parser IR in a form that's useable by the rest of the compiler. For
 * that reason, any analyses or transformations that need to take that
 * information into account need to run before this pass.
 *
 * @pre The parser and deparser IR have been simplified by
 * ResolveComputedParserExpressions and PHV allocation has completed
 * successfully.
 *
 * @post The parser and deparser IR are replaced by lowered versions.
 */
class LowerParser : public Logging::PassManager {
 private:
    /// Containers in the original/non-lowered parser with zero inits
    std::map<gress_t, std::set<PHV::Container>> origParserZeroInitContainers;

 public:
    explicit LowerParser(const PhvInfo &phv, ClotInfo &clot, const FieldDefUse &defuse,
                         const ParserHeaderSequences &parserHeaderSeqs,
                         PhvLogging::CollectDefUseInfo *defuseInfo);
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_LOWER_PARSER_H_ */
