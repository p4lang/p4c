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

#include "bf-p4c/phv/pragma/phv_pragmas.h"

/// @returns true if for the associated \@pragmaName, the @p gress is either ingress or egress.
bool PHV::Pragmas::gressValid(cstring gress) { return gress == "ingress" || gress == "egress"; }

/**
 * Check if valid combination of pipe and gress arguments is passed.
 * @param[in]     exprs List of expressions
 * @param[in,out] expr_index Index of the first argument that is not pipe or gress
 * @param[in,out] required_args Minimal number of arguments without the pipe argument
 * @param[out]    pipe_arg Pointer to the pipe argument
 * @param[out]    gress_arg Pointer to the gress argument
 * @returns true if valid combination of pipe and gress arguments is found,
 *          false otherwise.
 */
bool PHV::Pragmas::determinePipeGressArgs(const IR::Vector<IR::Expression> &exprs,
                                          unsigned &expr_index, unsigned &required_args,
                                          const IR::StringLiteral *&pipe_arg,
                                          const IR::StringLiteral *&gress_arg) {
    if (exprs.empty()) {
        return false;
    }
    // At least one argument is present, ensured by grammar
    auto arg0 = exprs.at(expr_index++)->to<IR::StringLiteral>();

    // Determine whether the name of a pipe is present
    if (gressValid(arg0->value)) {
        // The first argument is gress so there is no pipe
        gress_arg = arg0;
    } else if (BFNContext::get().isPipeName(arg0->value)) {
        // The first argument is pipe, the second must be gress
        pipe_arg = arg0;
        required_args++;
        if (exprs.size() > 1) {
            // Ensure there is another argument
            gress_arg = exprs.at(expr_index++)->to<IR::StringLiteral>();
            if (!gressValid(gress_arg->value)) {
                warning(ErrorType::WARN_UNKNOWN,
                        "%1%: Found invalid gress argument. Ignoring pragma.", gress_arg);
                return false;
            }
        }
    } else {
        // The first argument is not gress neither pipe
        warning(ErrorType::WARN_UNKNOWN,
                "%1%: Found invalid gress or pipe argument. Ignoring pragma.", arg0);
        return false;
    }
    return true;
}

/**
 * Check if all arguments of the pragma are string literals.
 * @param[in]     exprs List of expressions
 * @returns true if all arguments of the pragma are string literals,
 *          false otherwise.
 */
bool PHV::Pragmas::checkStringLiteralArgs(const IR::Vector<IR::Expression> &exprs) {
    for (auto &expr : exprs) {
        if (!expr->is<IR::StringLiteral>()) {
            warning(ErrorType::WARN_INVALID,
                    "%1%: Found a non-string literal argument. Ignoring pragma.", expr);
            return false;
        }
    }
    return true;
}

/**
 * Check if the number of arguments is suitable for the specified pragma.
 * @param[in] annotation Annotation IR
 * @param[in] required_args Number of required arguments
 * @param[in] min_required_args Minimal number of required arguments
 * @param[in] exact_number_of_args Number of required arguments is exact or minimal
 * @param[in] pragma_name Pragma name
 * @param[in] pragma_args_wo_pipe Arguments of the pragma excluding pipe argument
 * @return Returns true if suitable number of arguments is passed, false otherwise.
 */
bool PHV::Pragmas::checkNumberArgs(const IR::Annotation *annotation, unsigned required_args,
                                   const unsigned min_required_args, bool exact_number_of_args,
                                   cstring pragma_name, cstring pragma_args_wo_pipe) {
    auto &exprs = annotation->expr;
    if ((exact_number_of_args && exprs.size() != required_args) || exprs.size() < required_args) {
        warning(ErrorType::WARN_INVALID,
                "%1%: Invalid number of arguments. "
                "With `pipe' and `gress' arguments, pragma @%2% requires %3% "
                "%4%arguments (`pipe', %5%%6%). "
                "Without `pipe' argument, pragma @%2% requires %7% "
                "%4%arguments (%5%%6%). "
                "Found %8% arguments. Ignoring pragma.",
                /* %1% */ annotation,
                /* %2% */ pragma_name,
                /* %3% */ min_required_args + 1,
                /* %4% */ exact_number_of_args ? "" : "or more ",
                /* %5% */ pragma_args_wo_pipe,
                /* %6% */ exact_number_of_args ? "" : ", ...",
                /* %7% */ min_required_args,
                /* %8% */ exprs.size());
        return false;
    }
    return true;
}

/**
 * Check whether the pragma should be applied in the specified pipe.
 * @param[in] annotation Pragma
 * @param[in] pipe Pipe IR argument
 * @param[in] pipe_arg Pipe argument delivered in the pragma
 * @return Returns true if the pragma should be applied in the specified pipe,
 *         false otherwise.
 */
bool PHV::Pragmas::checkPipeApplication(const IR::Annotation *annotation, const IR::BFN::Pipe *pipe,
                                        const IR::StringLiteral *pipe_arg) {
    if (pipe_arg && pipe->canon_name() && pipe_arg->value != pipe->canon_name()) {
        LOG4("Skipping pragma " << cstring::to_cstring(annotation) << " at the pipe `"
                                << pipe->canon_name() << "'.");
        return false;
    }
    return true;
}

/**
 * Report no matching PHV field.
 * @param[in] pipe Pipe IR. If null, pipe name is not shown.
 * @param[in] expr Expression IR with source info.
 * @param[in] field_name If @p expr is null, use this argument as the field name.
 */
void PHV::Pragmas::reportNoMatchingPHV(const IR::BFN::Pipe *pipe, const IR::Expression *expr,
                                       cstring field_name) {
    if (expr) {
        // DO NOT print extremely long expression.
        std::string expr_str = expr->toString().c_str();
        const int MAX_EXPR_SIZE = 200;
        if (expr_str.length() > MAX_EXPR_SIZE) {
            expr_str = expr_str.substr(0, 200) + "......";
        }
        if (pipe && pipe->canon_name()) {
            // If the pipe is named
            warning(ErrorType::WARN_INVALID,
                    "%1%: No matching PHV field in the pipe `%2%'. Ignoring pragma.", expr_str,
                    pipe->canon_name());
        } else {
            warning(ErrorType::WARN_INVALID, "%1%: No matching PHV field. Ignoring pragma.",
                    expr_str);
        }
    } else {
        // No source info available
        if (pipe && pipe->canon_name()) {
            // If the pipe is named
            warning(ErrorType::WARN_INVALID,
                    "No matching PHV field `%1' in the pipe `%2%'. Ignoring pragma.", field_name,
                    pipe->canon_name());
        } else {
            warning(ErrorType::WARN_INVALID, "No matching PHV field `%1%'. Ignoring pragma.",
                    field_name);
        }
    }
}
