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

#ifndef P4C_LIB_ERROR_CATALOG_H_
#define P4C_LIB_ERROR_CATALOG_H_

/// enumerate supported errors
/// Backends should extend this class with additional errors in the range 500-999 and
/// warnings in the range 1500-2141.
/// It is a class and not an enum class because in C++11 you can't extend an enum class
class ErrorType {
 public:
    // -------- Errors -------------
    // errors as initially defined with a format string
    static const int LEGACY_ERROR      = 0;
    static const int ERR_UNKNOWN       = 1;  // unknown construct (in context)
    static const int ERR_UNSUPPORTED   = 2;  // unsupported construct
    static const int ERR_UNEXPECTED    = 3;  // unexpected construct
    static const int ERR_UNINITIALIZED = 4;  // uninitialized reads/writes
    static const int ERR_EXPECTED      = 5;  // language, compiler expects a different construct
    static const int ERR_NOT_FOUND     = 6;  // A different way to say ERR_EXPECTED
    static const int ERR_INVALID       = 7;  // invalid construct
    static const int ERR_EXPRESSION    = 8;  // expression too complex, or other expression related errors
    static const int ERR_OVERLIMIT     = 9;  // program node exceeds target limits
    static const int ERR_INSUFFICIENT  = 10;  // program node does not have enough of ...

    // If we specialize for 1000 error types we're good!
    static const int ERR_MAX_ERRORS = 999;

    // -------- Warnings -----------
    // warnings as initially defined with a format string
    static const int LEGACY_WARNING = ERR_MAX_ERRORS + 1;
    static const int WARN_UNKNOWN     = 1001;  // unknown construct (in context)
    static const int WARN_UNSUPPORTED = 1002;  // unsupported construct
    static const int WARN_MISMATCH    = 1003;  // mismatched constructs

    static const int WARN_MAX_WARNINGS = 2142;
};

#endif  // P4C_LIB_ERROR_CATALOG_H_
