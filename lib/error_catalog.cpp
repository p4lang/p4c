/*
Copyright 2018-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "error_catalog.h"

#include <map>

#include "lib/cstring.h"

namespace P4 {

using namespace P4::literals;

// -------- Errors -------------
const int ErrorType::LEGACY_ERROR = 0;
const int ErrorType::ERR_UNKNOWN = 1;
const int ErrorType::ERR_UNSUPPORTED = 2;
const int ErrorType::ERR_UNEXPECTED = 3;
const int ErrorType::ERR_UNINITIALIZED = 4;
const int ErrorType::ERR_EXPECTED = 5;
const int ErrorType::ERR_NOT_FOUND = 6;
const int ErrorType::ERR_INVALID = 7;
const int ErrorType::ERR_EXPRESSION = 8;
const int ErrorType::ERR_OVERLIMIT = 9;
const int ErrorType::ERR_INSUFFICIENT = 10;
const int ErrorType::ERR_TYPE_ERROR = 11;
const int ErrorType::ERR_UNSUPPORTED_ON_TARGET = 12;
const int ErrorType::ERR_DUPLICATE = 13;
const int ErrorType::ERR_IO = 14;
const int ErrorType::ERR_UNREACHABLE = 15;
const int ErrorType::ERR_MODEL = 16;
const int ErrorType::ERR_RESERVED = 17;

// ------ Warnings -----------
const int ErrorType::LEGACY_WARNING = ERR_MAX + 1;
const int ErrorType::WARN_FAILED = 1001;
const int ErrorType::WARN_UNKNOWN = 1002;
const int ErrorType::WARN_INVALID = 1003;
const int ErrorType::WARN_UNSUPPORTED = 1004;
const int ErrorType::WARN_DEPRECATED = 1005;
const int ErrorType::WARN_UNINITIALIZED = 1006;
const int ErrorType::WARN_UNUSED = 1007;
const int ErrorType::WARN_MISSING = 1008;
const int ErrorType::WARN_ORDERING = 1009;
const int ErrorType::WARN_MISMATCH = 1010;
const int ErrorType::WARN_OVERFLOW = 1011;
const int ErrorType::WARN_IGNORE_PROPERTY = 1012;
const int ErrorType::WARN_TYPE_INFERENCE = 1013;
const int ErrorType::WARN_PARSER_TRANSITION = 1014;
const int ErrorType::WARN_UNREACHABLE = 1015;
const int ErrorType::WARN_SHADOWING = 1016;
const int ErrorType::WARN_IGNORE = 1017;
const int ErrorType::WARN_UNINITIALIZED_OUT_PARAM = 1018;
const int ErrorType::WARN_UNINITIALIZED_USE = 1019;
const int ErrorType::WARN_INVALID_HEADER = 1020;
const int ErrorType::WARN_DUPLICATE_PRIORITIES = 1021;
const int ErrorType::WARN_ENTRIES_OUT_OF_ORDER = 1022;
const int ErrorType::WARN_MULTI_HDR_EXTRACT = 1023;
const int ErrorType::WARN_EXPRESSION = 1024;
const int ErrorType::WARN_DUPLICATE = 1025;

// ------ Info messages -----------
const int ErrorType::INFO_INFERRED = WARN_MAX + 1;
const int ErrorType::INFO_PROGRESS = 2143;

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
    {ErrorType::ERR_IO, "I/O error"_cs},
    {ErrorType::ERR_MODEL, "Target model error"_cs},
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
    {ErrorType::WARN_UNREACHABLE, "parser-transition"_cs},
    {ErrorType::WARN_SHADOWING, "shadow"_cs},
    {ErrorType::WARN_UNINITIALIZED_USE, "uninitialized_use"_cs},
    {ErrorType::WARN_UNINITIALIZED_OUT_PARAM, "uninitialized_out_param"_cs},
    {ErrorType::WARN_IGNORE, "ignore"_cs},
    {ErrorType::WARN_INVALID_HEADER, "invalid_header"_cs},
    {ErrorType::WARN_DUPLICATE_PRIORITIES, "duplicate_priorities"_cs},
    {ErrorType::WARN_ENTRIES_OUT_OF_ORDER, "entries_out_of_priority_order"_cs},
    {ErrorType::WARN_MULTI_HDR_EXTRACT, "multi_header_extract"_cs},
    {ErrorType::WARN_EXPRESSION, "expr"_cs},
    {ErrorType::WARN_DUPLICATE, "duplicate"_cs},

    // Info messages
    {ErrorType::INFO_INFERRED, "inferred"_cs},
    {ErrorType::INFO_PROGRESS, "progress"_cs}};

}  // namespace P4
