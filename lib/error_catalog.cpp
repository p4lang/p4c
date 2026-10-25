// SPDX-FileCopyrightText: 2018 Barefoot Networks, Inc.
// Copyright 2018-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "error_catalog.h"

#include <map>

#include "error_reporter.h"
#include "lib/cstring.h"

namespace P4 {

using namespace P4::literals;

// map from errorCode to ErrorSig
std::map<int, cstring> ErrorCatalog::errorCatalog = {
    // Errors
    {ErrorType::LEGACY_ERROR, "legacy"_cs},
    {ErrorType::ERR_UNKNOWN, "unknown"_cs},
    {ErrorType::ERR_UNSUPPORTED, "unsupported"_cs},
    {ErrorType::ERR_UNEXPECTED, "unexpected"_cs},
    {ErrorType::ERR_EXPECTED, "expected"_cs},
    {ErrorType::ERR_NOT_FOUND, "not-found"_cs},
    {ErrorType::ERR_INVALID, "invalid"_cs},
    {ErrorType::ERR_EXPRESSION, "expr"_cs},
    {ErrorType::ERR_OVERLIMIT, "overlimit"_cs},
    {ErrorType::ERR_INSUFFICIENT, "insufficient"_cs},
    {ErrorType::ERR_UNINITIALIZED, "uninitialized"_cs},
    {ErrorType::ERR_TYPE_ERROR, "type-error"_cs},
    {ErrorType::ERR_UNSUPPORTED_ON_TARGET, "target-error"_cs},
    {ErrorType::ERR_DUPLICATE, "duplicate"_cs},
    {ErrorType::ERR_UNREACHABLE, "unreachable"_cs},
    {ErrorType::ERR_IO, "I/O error"_cs},
    {ErrorType::ERR_MODEL, "Target model error"_cs},
    {ErrorType::ERR_TABLE_KEYS, "keys"_cs},
    {ErrorType::ERR_RESERVED, "reserved"_cs},

    // Warnings
    {ErrorType::LEGACY_WARNING, "legacy"_cs},
    {ErrorType::WARN_FAILED, "failed"_cs},
    {ErrorType::WARN_UNKNOWN, "unknown"_cs},
    {ErrorType::WARN_INVALID, "invalid"_cs},
    {ErrorType::WARN_UNSUPPORTED, "unsupported"_cs},
    {ErrorType::WARN_DEPRECATED, "deprecated"_cs},
    {ErrorType::WARN_UNINITIALIZED, "uninitialized"_cs},
    {ErrorType::WARN_UNUSED, "unused"_cs},
    {ErrorType::WARN_MISSING, "missing"_cs},
    {ErrorType::WARN_ORDERING, "ordering"_cs},
    {ErrorType::WARN_MISMATCH, "mismatch"_cs},
    {ErrorType::WARN_OVERFLOW, "overflow"_cs},
    {ErrorType::WARN_IGNORE_PROPERTY, "ignore-prop"_cs},
    {ErrorType::WARN_TYPE_INFERENCE, "type-inference"_cs},
    {ErrorType::WARN_PARSER_TRANSITION, "parser-transition"_cs},
    {ErrorType::WARN_UNREACHABLE, "unreachable"_cs},
    {ErrorType::WARN_SHADOWING, "shadow"_cs},
    {ErrorType::WARN_UNINITIALIZED_USE, "uninitialized-use"_cs},
    {ErrorType::WARN_UNINITIALIZED_OUT_PARAM, "uninitialized-out-param"_cs},
    {ErrorType::WARN_IGNORE, "ignore"_cs},
    {ErrorType::WARN_INVALID_HEADER, "invalid-header"_cs},
    {ErrorType::WARN_DUPLICATE_PRIORITIES, "duplicate-priorities"_cs},
    {ErrorType::WARN_ENTRIES_OUT_OF_ORDER, "entries-out-of-priority-order"_cs},
    {ErrorType::WARN_MULTI_HDR_EXTRACT, "multi-header-extract"_cs},
    {ErrorType::WARN_EXPRESSION, "expr"_cs},
    {ErrorType::WARN_DUPLICATE, "duplicate"_cs},
    {ErrorType::WARN_BRANCH_HINT, "branch"_cs},
    {ErrorType::WARN_TABLE_KEYS, "keys"_cs},

    // Info messages
    {ErrorType::INFO_INFERRED, "inferred"_cs},
    {ErrorType::INFO_PROGRESS, "progress"_cs},
    {ErrorType::INFO_REMOVED, "removed"_cs}};

void ErrorCatalog::initReporter(ErrorReporter &reporter) {
    // by default, ignore warnings about branch hints -- user can turn them
    // on with --Wwarn=branch
    reporter.setDiagnosticAction("branch"_cs, DiagnosticAction::Ignore);
}

}  // namespace P4
