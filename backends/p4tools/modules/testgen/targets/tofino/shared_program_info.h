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

#ifndef TESTGEN_TARGETS_TOFINO_SHARED_PROGRAM_INFO_H_
#define TESTGEN_TARGETS_TOFINO_SHARED_PROGRAM_INFO_H_

#include <vector>

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/targets/tofino/compiler_result.h"

namespace P4::P4Tools::P4Testgen::Tofino {

class TofinoSharedProgramInfo : public ProgramInfo {
 public:
    using PipeInfo = struct {
        cstring pipeName;
        ordered_map<cstring, const IR::Type_Declaration *> pipes;
    };

 protected:
    /// The parser, MAU, and deparser, for each gress and pipe in the program.
    const std::vector<PipeInfo> pipes;

    /// The declid of each top-level parser type, mapped to its corresponding gress.
    const std::map<int, gress_t> declIdToGress;

    /// The declid of each top-level parser type, mapped to its corresponding Tofino pipe.
    const std::map<int, size_t> declIdToPipe;

    /// The bit width of standard_metadata.parser_error.
    static const IR::Type_Bits parserErrBits;

 public:
    explicit TofinoSharedProgramInfo(const TofinoCompilerResult &compilerResult,
                                     std::vector<PipeInfo> inputPipes,
                                     std::map<int, gress_t> declIdToGress,
                                     std::map<int, size_t> declIdToPipe);

    /// @returns the gress associated with the given top level control/parser declaration.
    gress_t getGress(const IR::Type_Declaration *decl) const;

    /// @returns the pipe associated with the given top level control/parser declaration.
    size_t getPipeIdx(const IR::Type_Declaration *decl) const;

    // @returns whether the current program is a multi-pipe program.
    [[nodiscard]] bool isMultiPipe() const;

    /// @returns the table associated with the direct extern
    const IR::P4Table *getTableofDirectExtern(const IR::IDeclaration *directExternDecl) const;

    /// @returns the top-level parser/control sequence.
    [[nodiscard]] const std::vector<PipeInfo> *getPipes() const;

    /// @returns set of commands which constitute ingress pipeline.
    [[nodiscard]] virtual std::vector<std::vector<Continuation::Command>> ingressCmds() const = 0;

    /// @returns set of commands which constitute egress pipeline.
    [[nodiscard]] virtual std::vector<std::vector<Continuation::Command>> egressCmds() const = 0;

    /// @returns expression which limits which ports can be used, expressed as a constraint
    /// over @p portVar
    static const IR::Expression *getBackendConstraint(const IR::StateVariable &portVar);

    [[nodiscard]] virtual const IR::Expression *getValidPortConstraint(
        const IR::StateVariable &portVar) const = 0;

    [[nodiscard]] virtual std::optional<const IR::Expression *> getPipePortRangeConstraint(
        const IR::StateVariable &portVar, size_t pipeIdx) const = 0;

    IR::StateVariable getParserParamVar(const IR::P4Parser *parser, const IR::Type *type,
                                        size_t paramIndex, cstring paramLabel) const;

    [[nodiscard]] const IR::Type_Bits *getParserErrorType() const override;

    [[nodiscard]] const TofinoCompilerResult &getCompilerResult() const override;

    DECLARE_TYPEINFO(TofinoSharedProgramInfo, ProgramInfo);
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* TESTGEN_TARGETS_TOFINO_SHARED_PROGRAM_INFO_H_ */
