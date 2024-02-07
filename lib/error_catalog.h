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

#ifndef LIB_ERROR_CATALOG_H_
#define LIB_ERROR_CATALOG_H_

#include <map>
#include <string>

#include "cstring.h"
#include "lib/error_message.h"
#include "lib/exceptions.h"

using MessageType = ErrorMessage::MessageType;

/// enumerate supported errors
/// It is a class and not an enum class because in C++11 you can't extend an enum class
class ErrorType {
 public:
    // -------- Errors -------------
    // errors as initially defined with a format string
    static const int LEGACY_ERROR;
    static const int ERR_UNKNOWN;                // unknown construct (in context)
    static const int ERR_UNSUPPORTED;            // unsupported construct
    static const int ERR_UNEXPECTED;             // unexpected construct
    static const int ERR_UNINITIALIZED;          // uninitialized reads/writes
    static const int ERR_EXPECTED;               // language, compiler expects a different construct
    static const int ERR_NOT_FOUND;              // A different way to say ERR_EXPECTED
    static const int ERR_INVALID;                // invalid construct
    static const int ERR_EXPRESSION;             // expression related errors
    static const int ERR_OVERLIMIT;              // program node exceeds target limits
    static const int ERR_INSUFFICIENT;           // program node does not have enough of ...
    static const int ERR_TYPE_ERROR;             // P4 type checking errors
    static const int ERR_UNSUPPORTED_ON_TARGET;  // target can not handle construct
    static const int ERR_DUPLICATE;              // duplicate objects
    static const int ERR_IO;                     // IO error
    static const int ERR_UNREACHABLE;            // unreachable parser state
    static const int ERR_MODEL;                  // something is wrong with the target model
    static const int ERR_RESERVED;               // Reserved for target use
    // Backends should extend this class with additional errors in the range 500-999.
    static const int ERR_MIN_BACKEND = 500;  // first allowed backend error code
    static const int ERR_MAX = 999;          // last allowed error code

    // -------- Warnings -----------
    // warnings as initially defined with a format string
    static const int LEGACY_WARNING;
    static const int WARN_FAILED;                   // non-fatal failure!
    static const int WARN_UNKNOWN;                  // unknown construct (in context)
    static const int WARN_INVALID;                  // invalid construct
    static const int WARN_UNSUPPORTED;              // unsupported construct
    static const int WARN_DEPRECATED;               // deprecated feature
    static const int WARN_UNINITIALIZED;            // unitialized instance
    static const int WARN_UNINITIALIZED_USE;        // use of uninitialized value
    static const int WARN_UNINITIALIZED_OUT_PARAM;  // output parameter may be uninitialized
    static const int WARN_UNUSED;                   // unused instance
    static const int WARN_MISSING;                  // missing construct
    static const int WARN_ORDERING;                 // inconsistent statement ordering
    static const int WARN_MISMATCH;                 // mismatched constructs
    static const int WARN_OVERFLOW;                 // values do not fit
    static const int WARN_IGNORE_PROPERTY;          // invalid property for object, ignored
    static const int WARN_TYPE_INFERENCE;           // type inference can not infer, substitutes
    static const int WARN_PARSER_TRANSITION;        // parser transition non-fatal issues
    static const int WARN_UNREACHABLE;              // parser state unreachable
    static const int WARN_SHADOWING;                // instance shadowing
    static const int WARN_IGNORE;                   // simply ignore
    static const int WARN_INVALID_HEADER;           // access to fields of an invalid header
    static const int WARN_DUPLICATE_PRIORITIES;     // two entries with the same priority
    static const int WARN_ENTRIES_OUT_OF_ORDER;     // entries with priorities out of order
    // Backends should extend this class with additional warnings in the range 1500-2141.
    static const int WARN_MIN_BACKEND = 1500;  // first allowed backend warning code
    static const int WARN_MAX = 2141;          // last allowed warning code

    // -------- Info messages -------------
    // info messages as initially defined with a format string
    static const int INFO_INFERRED;  // information inferred by compiler
    static const int INFO_PROGRESS;  // compilation progress

    // Backends should extend this class with additional info messages in the range 3000-3999.
    static const int INFO_MIN_BACKEND = 3000;  // first allowed backend info code
    static const int INFO_MAX = 3999;          // last allowed info code
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
    /// @param type      - error/warning/info message
    /// @param errorCode - integer value for the error/warning
    /// @param name      - name for the error. Used to enable/disable all errors of that type
    /// @param forceReplace - override an existing error type in the catalog
    template <MessageType type, int errorCode>
    bool add(const char *name, bool forceReplace = false) {
        static_assert(type != MessageType::Error ||
                      (errorCode >= ErrorType::ERR_MIN_BACKEND && errorCode <= ErrorType::ERR_MAX));
        static_assert(type != MessageType::Warning || (errorCode >= ErrorType::WARN_MIN_BACKEND &&
                                                       errorCode <= ErrorType::WARN_MAX));
        static_assert(type != MessageType::Info || (errorCode >= ErrorType::INFO_MIN_BACKEND &&
                                                    errorCode <= ErrorType::INFO_MAX));
        static_assert(type != MessageType::None);
        if (forceReplace) errorCatalog.erase(errorCode);
        auto it = errorCatalog.emplace(errorCode, name);
        return it.second;
    }

    /// retrieve the name for errorCode
    cstring getName(int errorCode) {
        if (errorCatalog.count(errorCode)) return errorCatalog.at(errorCode);
        return "--unknown--";
    }

    /// return true if the given diagnostic can _only_ be an error; false otherwise
    bool isError(cstring name) {
        // Some diagnostics might be both errors and warning/info
        // (e.g. "invalid" -> both ERR_INVALID and WARN_INVALID).
        bool error = false;
        for (const auto &pair : errorCatalog) {
            if (pair.second == name) {
                if (pair.first < ErrorType::LEGACY_ERROR || pair.first > ErrorType::ERR_MAX)
                    return false;
                error = true;
            }
        }

        return error;
    }

 private:
    ErrorCatalog() {}

    /// map from errorCode to name
    static std::map<int, cstring> errorCatalog;
};

#endif /* LIB_ERROR_CATALOG_H_ */
