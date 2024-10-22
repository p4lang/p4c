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

#ifndef BF_P4C_PARDE_EXTRACT_PARSER_H_
#define BF_P4C_PARDE_EXTRACT_PARSER_H_

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "logging/pass_manager.h"

namespace P4 {
namespace IR {

namespace BFN {
class Deparser;
class Parser;
class Pipe;
}  // namespace BFN

class P4Control;
class P4Parser;

}  // namespace IR
}  // namespace P4

namespace BFN {

using namespace P4;
/**
 * @ingroup parde
 * @brief Transforms midend parser IR::BFN::TnaParser into backend parser IR::BFN::Parser
 */
class ExtractParser : public ParserInspector {
    Logging::FileLog *parserLog = nullptr;

 public:
    explicit ExtractParser(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, IR::BFN::Pipe *rv,
                           ParseTna *arch)
        : refMap(refMap), typeMap(typeMap), rv(rv), arch(arch) {
        setName("ExtractParser");
    }
    void postorder(const IR::BFN::TnaParser *parser) override;
    void end_apply() override;

    profile_t init_apply(const IR::Node *root) override {
        if (BackendOptions().verbose > 0)
            parserLog = new Logging::FileLog(rv->canon_id(), "parser.log"_cs);
        return ParserInspector::init_apply(root);
    }

 private:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    IR::BFN::Pipe *rv;
    ParseTna *arch;
};

/// Process IR::BFN::Parser and IR::BFN::Deparser to resolve header stack and add shim
/// for intrinsic metadata, must be applied to IR::BFN::Parser and IR::BFN::Deparser.
class ProcessParde : public Logging::PassManager {
 public:
    ProcessParde(const IR::BFN::Pipe *rv, bool useTna);
};

}  // namespace BFN

#endif /* BF_P4C_PARDE_EXTRACT_PARSER_H_ */
