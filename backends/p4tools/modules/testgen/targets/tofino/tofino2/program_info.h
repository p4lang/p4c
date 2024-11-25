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

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TOFINO2_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TOFINO2_PROGRAM_INFO_H_

#include <cstddef>
#include <map>
#include <vector>

#include "backends/tofino/bf-p4c/ir/gress.h"
#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/targets/tofino/shared_program_info.h"

namespace P4::P4Tools::P4Testgen::Tofino {

class JBayProgramInfo : public TofinoSharedProgramInfo {
 private:
    /// This function contains an imperative specification of the inter-pipe interaction in the
    /// target.
    std::vector<Continuation::Command> processDeclaration(const IR::Type_Declaration *typeDecl,
                                                          size_t pipeIdx) const;

    [[nodiscard]] std::vector<std::vector<Continuation::Command>> pipelineCmds(gress_t gress) const;

    [[nodiscard]] const IR::Expression *getValidPortConstraint(
        const IR::StateVariable &portVar) const override;

    [[nodiscard]] std::optional<const IR::Expression *> getPipePortRangeConstraint(
        const IR::StateVariable &portVar, size_t pipeIdx) const override;

 public:
    JBayProgramInfo(const TofinoCompilerResult &compilerResult, std::vector<PipeInfo> inputPipes,
                    std::map<int, gress_t> declIdToGress, std::map<int, size_t> declIdToPipe);

    /// @see ProgramInfo::getArchSpec
    [[nodiscard]] const ArchSpec &getArchSpec() const override;

    [[nodiscard]] const IR::StateVariable &getTargetInputPortVar() const override;

    [[nodiscard]] const IR::StateVariable &getTargetOutputPortVar() const override;

    [[nodiscard]] const IR::Expression *dropIsActive() const override;

    [[nodiscard]] std::vector<std::vector<Continuation::Command>> ingressCmds() const override;

    [[nodiscard]] std::vector<std::vector<Continuation::Command>> egressCmds() const override;

    /// @see ProgramInfo::getArchSpec
    static const ArchSpec ARCH_SPEC;

    DECLARE_TYPEINFO(JBayProgramInfo, TofinoSharedProgramInfo);
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TOFINO2_PROGRAM_INFO_H_ */
