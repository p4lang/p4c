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

#include <map>
#include <string>
#include "error_catalog.h"

// -------- Errors -------------
const int ErrorType::LEGACY_ERROR      =   0;
const int ErrorType::ERR_UNKNOWN       =   1;
const int ErrorType::ERR_UNSUPPORTED   =   2;
const int ErrorType::ERR_UNEXPECTED    =   3;
const int ErrorType::ERR_UNINITIALIZED =   4;
const int ErrorType::ERR_EXPECTED      =   5;
const int ErrorType::ERR_NOT_FOUND     =   6;
const int ErrorType::ERR_INVALID       =   7;
const int ErrorType::ERR_EXPRESSION    =   8;
const int ErrorType::ERR_OVERLIMIT     =   9;
const int ErrorType::ERR_INSUFFICIENT  =  10;
const int ErrorType::ERR_TYPE_ERROR    =  11;
const int ErrorType::ERR_UNSUPPORTED_ON_TARGET = 12;
const int ErrorType::ERR_DUPLICATE = 13;
// If we specialize for 1000 error types we're good!
const int ErrorType::ERR_MAX_ERRORS    = 999;

// ------ Warnings -----------
const int ErrorType::LEGACY_WARNING = ERR_MAX_ERRORS + 1;
const int ErrorType::WARN_FAILED            = 1001;
const int ErrorType::WARN_UNKNOWN           = 1002;
const int ErrorType::WARN_INVALID           = 1003;
const int ErrorType::WARN_UNSUPPORTED       = 1004;
const int ErrorType::WARN_DEPRECATED        = 1005;
const int ErrorType::WARN_UNINITIALIZED     = 1006;
const int ErrorType::WARN_UNUSED            = 1007;
const int ErrorType::WARN_MISSING           = 1008;
const int ErrorType::WARN_ORDERING          = 1009;
const int ErrorType::WARN_MISMATCH          = 1010;
const int ErrorType::WARN_OVERFLOW          = 1011;
const int ErrorType::WARN_IGNORE_PROPERTY   = 1012;
const int ErrorType::WARN_TYPE_INFERENCE    = 1013;
const int ErrorType::WARN_PARSER_TRANSITION = 1014;
const int ErrorType::WARN_UNREACHABLE       = 1015;
const int ErrorType::WARN_SHADOWING         = 1016;
const int ErrorType::WARN_IGNORE            = 1017;
const int ErrorType::WARN_MAX_WARNINGS      = 2142;

using ErrorSig = std::pair<const char *, const std::string>;

// map from errorCode to pairs of (name, format)
std::map<int, ErrorSig> ErrorCatalog::errorCatalog = {
    // Errors
    { ErrorType::LEGACY_ERROR,           ErrorSig("legacy", "")},
    { ErrorType::ERR_UNKNOWN,            ErrorSig("unknown", "%1%: Unknown error")},
    { ErrorType::ERR_UNSUPPORTED,        ErrorSig("unsupported", "%1%: Unsupported")},
    { ErrorType::ERR_UNEXPECTED,         ErrorSig("unexpected", "%1%: Unexpected")},
    { ErrorType::ERR_EXPECTED,           ErrorSig("expected", "%1%: Expected")},
    { ErrorType::ERR_NOT_FOUND,          ErrorSig("not-found", "%1%: Not found")},
    { ErrorType::ERR_INVALID,            ErrorSig("invalid", "%1%: Invalid")},
    { ErrorType::ERR_EXPRESSION,         ErrorSig("expr", "%1%: Expression")},
    { ErrorType::ERR_OVERLIMIT,          ErrorSig("overlimit", "")},
    { ErrorType::ERR_INSUFFICIENT,       ErrorSig("insufficient", "%1%: Target requires")},
    { ErrorType::ERR_UNINITIALIZED,      ErrorSig("uninitialized", "%1%: Uninitialized")},
    { ErrorType::ERR_TYPE_ERROR,         ErrorSig("type-error", "")},
    { ErrorType::ERR_UNSUPPORTED_ON_TARGET, ErrorSig("target-error",
                                                     "%1%: Unsupported on target")},
    { ErrorType::ERR_DUPLICATE,          ErrorSig("duplicate", "")},

    // Warnings
    { ErrorType::LEGACY_WARNING,         ErrorSig("legacy", "")},
    { ErrorType::WARN_FAILED,            ErrorSig("failed", "")},
    { ErrorType::WARN_UNKNOWN,           ErrorSig("unknown", "")},
    { ErrorType::WARN_INVALID,           ErrorSig("invalid", "")},
    { ErrorType::WARN_UNSUPPORTED,       ErrorSig("unsupported", "")},
    { ErrorType::WARN_DEPRECATED,        ErrorSig("deprecated", "")},
    { ErrorType::WARN_UNINITIALIZED,     ErrorSig("uninitialized", "")},
    { ErrorType::WARN_UNUSED,            ErrorSig("unused", "")},
    { ErrorType::WARN_MISSING,           ErrorSig("missing", "")},
    { ErrorType::WARN_ORDERING,          ErrorSig("ordering", "")},
    { ErrorType::WARN_MISMATCH,          ErrorSig("mismatch", "")},
    { ErrorType::WARN_OVERFLOW,          ErrorSig("overflow", "")},
    { ErrorType::WARN_IGNORE_PROPERTY,   ErrorSig("ignore-prop", "")},
    { ErrorType::WARN_TYPE_INFERENCE,    ErrorSig("type-inference", "")},
    { ErrorType::WARN_PARSER_TRANSITION, ErrorSig("parser-transition", "")},
    { ErrorType::WARN_UNREACHABLE,       ErrorSig("parser-transition", "")},
    { ErrorType::WARN_SHADOWING,         ErrorSig("shadow", "")},
    { ErrorType::WARN_IGNORE,            ErrorSig("ignore", "")},
};
