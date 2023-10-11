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
#include <string>

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
// If we specialize for 1000 error types we're good!
const int ErrorType::ERR_MAX_ERRORS = 999;

// ------ Warnings -----------
const int ErrorType::LEGACY_WARNING = ERR_MAX_ERRORS + 1;
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
const int ErrorType::WARN_MAX_WARNINGS = 2142;

// ------ Info message -----------
const int ErrorType::INFO_INFERRED = WARN_MAX_WARNINGS + 1;
const int ErrorType::INFO_MAX_INFOS = 3999;

// map from errorCode to ErrorSig
std::map<int, cstring> ErrorCatalog::errorCatalog = {
    // Errors
    {ErrorType::LEGACY_ERROR, "legacy"},
    {ErrorType::ERR_UNKNOWN, "unknown"},
    {ErrorType::ERR_UNSUPPORTED, "unsupported"},
    {ErrorType::ERR_UNEXPECTED, "unexpected"},
    {ErrorType::ERR_EXPECTED, "expected"},
    {ErrorType::ERR_NOT_FOUND, "not-found"},
    {ErrorType::ERR_INVALID, "invalid"},
    {ErrorType::ERR_EXPRESSION, "expr"},
    {ErrorType::ERR_OVERLIMIT, "overlimit"},
    {ErrorType::ERR_INSUFFICIENT, "insufficient"},
    {ErrorType::ERR_UNINITIALIZED, "uninitialized"},
    {ErrorType::ERR_TYPE_ERROR, "type-error"},
    {ErrorType::ERR_UNSUPPORTED_ON_TARGET, "target-error"},
    {ErrorType::ERR_DUPLICATE, "duplicate"},
    {ErrorType::ERR_IO, "I/O error"},
    {ErrorType::ERR_MODEL, "Target model error"},
    {ErrorType::ERR_RESERVED, "reserved"},

    // Warnings
    {ErrorType::LEGACY_WARNING, "legacy"},
    {ErrorType::WARN_FAILED, "failed"},
    {ErrorType::WARN_UNKNOWN, "unknown"},
    {ErrorType::WARN_INVALID, "invalid"},
    {ErrorType::WARN_UNSUPPORTED, "unsupported"},
    {ErrorType::WARN_DEPRECATED, "deprecated"},
    {ErrorType::WARN_UNINITIALIZED, "uninitialized"},
    {ErrorType::WARN_UNUSED, "unused"},
    {ErrorType::WARN_MISSING, "missing"},
    {ErrorType::WARN_ORDERING, "ordering"},
    {ErrorType::WARN_MISMATCH, "mismatch"},
    {ErrorType::WARN_OVERFLOW, "overflow"},
    {ErrorType::WARN_IGNORE_PROPERTY, "ignore-prop"},
    {ErrorType::WARN_TYPE_INFERENCE, "type-inference"},
    {ErrorType::WARN_PARSER_TRANSITION, "parser-transition"},
    {ErrorType::WARN_UNREACHABLE, "parser-transition"},
    {ErrorType::WARN_SHADOWING, "shadow"},
    {ErrorType::WARN_UNINITIALIZED_USE, "uninitialized_use"},
    {ErrorType::WARN_UNINITIALIZED_OUT_PARAM, "uninitialized_out_param"},
    {ErrorType::WARN_IGNORE, "ignore"},
    {ErrorType::WARN_INVALID_HEADER, "invalid_header"},
    {ErrorType::WARN_DUPLICATE_PRIORITIES, "duplicate_priorities"},
    {ErrorType::WARN_ENTRIES_OUT_OF_ORDER, "entries_out_of_priority_order"},

    // Info messages
    {ErrorType::INFO_INFERRED, "inferred"}};
