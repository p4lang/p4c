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

#include <map>
#include <string>

/// enumerate supported errors
/// Backends should extend this class with additional errors in the range 500-999 and
/// warnings in the range 1500-2141.
/// It is a class and not an enum class because in C++11 you can't extend an enum class
class ErrorType {
 public:
    // -------- Errors -------------
    // errors as initially defined with a format string
    static const int LEGACY_ERROR;
    static const int ERR_UNKNOWN;             // unknown construct (in context)
    static const int ERR_UNSUPPORTED;         // unsupported construct
    static const int ERR_UNEXPECTED;          // unexpected construct
    static const int ERR_UNINITIALIZED;       // uninitialized reads/writes
    static const int ERR_EXPECTED;            // language, compiler expects a different construct
    static const int ERR_NOT_FOUND;           // A different way to say ERR_EXPECTED
    static const int ERR_INVALID;             // invalid construct
    static const int ERR_EXPRESSION;          // expression related errors
    static const int ERR_OVERLIMIT;           // program node exceeds target limits
    static const int ERR_INSUFFICIENT;        // program node does not have enough of ...
    static const int ERR_TYPE_ERROR;          // P4 type checking errors
    static const int ERR_UNSUPPORTED_ON_TARGET;  // target can not handle construct

    // If we specialize for 1000 error types we're good!
    static const int ERR_MAX_ERRORS;

    // -------- Warnings -----------
    // warnings as initially defined with a format string
    static const int LEGACY_WARNING;
    static const int WARN_FAILED;             // non-fatal failure!
    static const int WARN_UNKNOWN;            // unknown construct (in context)
    static const int WARN_INVALID;            // invalid construct
    static const int WARN_UNSUPPORTED;        // unsupported construct
    static const int WARN_DEPRECATED;         // deprecated feature
    static const int WARN_UNINITIALIZED;      // unitialized instance
    static const int WARN_UNUSED;             // unused instance
    static const int WARN_MISSING;            // missing construct
    static const int WARN_ORDERING;           // inconsistent statement ordering
    static const int WARN_MISMATCH;           // mismatched constructs
    static const int WARN_OVERFLOW;           // values do not fit
    static const int WARN_IGNORE_PROPERTY;    // invalid property for object, ignored
    static const int WARN_TYPE_INFERENCE;     // type inference can not infer, substitutes
    static const int WARN_PARSER_TRANSITION;  // parser transition non-fatal issues
    static const int WARN_UNREACHABLE;        // parser state unreachable
    static const int WARN_SHADOWING;          // instance shadowing
    static const int WARN_IGNORE;             // simply ignore

    static const int WARN_MAX_WARNINGS;
};

class ErrorCatalog {
 public:
    /// Return the singleton object
    static ErrorCatalog &getCatalog() {
        static ErrorCatalog instance;
        return instance;
    }

    /// add to the catalog
    /// returns false if the code already exists and forceReplace was not set to true
    /// @param errorCode - integer value for the error/warning
    /// @param name      - name for the error. Used to enable/disable all errors of that type
    /// @param fmt       - error string format
    /// @param forceReplace - override an existing error type in the catalog
    bool add(int errorCode, const char *name, const std::string fmt, bool forceReplace = false) {
        if (forceReplace)
            errorCatalog.erase(errorCode);
        auto it = errorCatalog.emplace(errorCode, std::pair<const char *, std::string>(name, fmt));
        return it.second;
    }

    /// add to the catalog
    /// returns false if the code already exists and forceReplace was not set to true
    bool add(int errorCode, const char *name, const char *fmt, bool forceReplace = false) {
        return add(errorCode, name, std::string(fmt), forceReplace);
    }

    /// retrieve the name for errorCode
    const char *getName(int errorCode) {
        if (errorCatalog.count(errorCode))
            return errorCatalog.at(errorCode).first;
        return "--unknown--";
    }

    /// retrieve the message format for errorCode
    const char *getFormat(int errorCode) {
        if (errorCatalog.count(errorCode))
            return errorCatalog.at(errorCode).second.c_str();
        static std::string msg("errorCatalog message not set for error code ");
        msg += std::to_string(errorCode);
        return msg.c_str();
    }

 private:
    ErrorCatalog() {}

    /// map from errorCode to pairs of (name, format)
    static std::map<int, std::pair<const char *, const std::string>> errorCatalog;
};

#endif  // P4C_LIB_ERROR_CATALOG_H_
