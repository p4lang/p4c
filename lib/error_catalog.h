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

namespace P4 {

using MessageType = ErrorMessage::MessageType;

class ErrorReporter;

/// enumerate supported errors
/// It is a class and not an enum class because in C++11 you can't extend an enum class
class ErrorType {
 public:
    // -------- Errors -------------
    // errors as initially defined with a format string
    static constexpr int LEGACY_ERROR = 0;
    static constexpr int ERR_UNKNOWN = 1;        // unknown construct (in context)
    static constexpr int ERR_UNSUPPORTED = 2;    // unsupported construct
    static constexpr int ERR_UNEXPECTED = 3;     // unexpected construct
    static constexpr int ERR_UNINITIALIZED = 4;  // uninitialized reads/writes
    static constexpr int ERR_EXPECTED = 5;       // language, compiler expects a different construct
    static constexpr int ERR_NOT_FOUND = 6;      // A different way to say ERR_EXPECTED
    static constexpr int ERR_INVALID = 7;        // invalid construct
    static constexpr int ERR_EXPRESSION = 8;     // expression related errors
    static constexpr int ERR_OVERLIMIT = 9;      // program node exceeds target limits
    static constexpr int ERR_INSUFFICIENT = 10;  // program node does not have enough of ...
    static constexpr int ERR_TYPE_ERROR = 11;    // P4 type checking errors
    static constexpr int ERR_UNSUPPORTED_ON_TARGET = 12;  // target can not handle construct
    static constexpr int ERR_DUPLICATE = 13;              // duplicate objects
    static constexpr int ERR_IO = 14;                     // IO error
    static constexpr int ERR_UNREACHABLE = 15;            // unreachable code
    static constexpr int ERR_MODEL = 16;       // something is wrong with the target model
    static constexpr int ERR_TABLE_KEYS = 17;  // something is wrong with a table key
    static constexpr int ERR_RESERVED = 18;    // Reserved for target use
    // Backends should extend this class with additional errors in the range 500-999.
    static constexpr int ERR_MIN_BACKEND = 500;  // first allowed backend error code
    static constexpr int ERR_MAX = 999;          // last allowed error code

    // -------- Warnings -----------
    // warnings as initially defined with a format string
    static constexpr int LEGACY_WARNING = ERR_MAX + 1;
    static constexpr int WARN_FAILED = 1001;             // non-fatal failure!
    static constexpr int WARN_UNKNOWN = 1002;            // unknown construct (in context)
    static constexpr int WARN_INVALID = 1003;            // invalid construct
    static constexpr int WARN_UNSUPPORTED = 1004;        // unsupported construct
    static constexpr int WARN_DEPRECATED = 1005;         // deprecated feature
    static constexpr int WARN_UNINITIALIZED = 1006;      // unitialized instance
    static constexpr int WARN_UNINITIALIZED_USE = 1019;  // use of uninitialized value
    static constexpr int WARN_UNINITIALIZED_OUT_PARAM =
        1018;                                          // output parameter may be uninitialized
    static constexpr int WARN_UNUSED = 1007;           // unused instance
    static constexpr int WARN_MISSING = 1008;          // missing construct
    static constexpr int WARN_ORDERING = 1009;         // inconsistent statement ordering
    static constexpr int WARN_MISMATCH = 1010;         // mismatched constructs
    static constexpr int WARN_OVERFLOW = 1011;         // values do not fit
    static constexpr int WARN_IGNORE_PROPERTY = 1012;  // invalid property for object, ignored
    static constexpr int WARN_TYPE_INFERENCE = 1013;   // type inference can not infer, substitutes
    static constexpr int WARN_PARSER_TRANSITION = 1014;     // parser transition non-fatal issues
    static constexpr int WARN_UNREACHABLE = 1015;           // unreachable code
    static constexpr int WARN_SHADOWING = 1016;             // instance shadowing
    static constexpr int WARN_IGNORE = 1017;                // simply ignore
    static constexpr int WARN_INVALID_HEADER = 1020;        // access to fields of an invalid header
    static constexpr int WARN_DUPLICATE_PRIORITIES = 1021;  // two entries with the same priority
    static constexpr int WARN_ENTRIES_OUT_OF_ORDER = 1022;  // entries with priorities out of order
    static constexpr int WARN_MULTI_HDR_EXTRACT =
        1023;                                      // same header may be extracted more than once
    static constexpr int WARN_EXPRESSION = 1024;   // expression related warnings
    static constexpr int WARN_DUPLICATE = 1025;    // duplicate objects
    static constexpr int WARN_BRANCH_HINT = 1026;  // branch frequency/likely hints
    static constexpr int WARN_TABLE_KEYS = 1027;   // something is wrong with a table key
    // Backends should extend this class with additional warnings in the range 1500-2141.
    static constexpr int WARN_MIN_BACKEND = 1500;  // first allowed backend warning code
    static constexpr int WARN_MAX = 2141;          // last allowed warning code

    // -------- Info messages -------------
    // info messages as initially defined with a format string
    static constexpr int INFO_INFERRED = WARN_MAX + 1;  // information inferred by compiler
    static constexpr int INFO_PROGRESS = 2143;          // compilation progress
    static constexpr int INFO_REMOVED = 2144;           // instance removed

    // Backends should extend this class with additional info messages in the range 3000-3999.
    static constexpr int INFO_MIN_BACKEND = 3000;  // first allowed backend info code
    static constexpr int INFO_MAX = 3999;          // last allowed info code
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
        using namespace P4::literals;

        if (errorCatalog.count(errorCode)) return errorCatalog.at(errorCode);
        return "--unknown--"_cs;
    }

    bool isError(int errorCode) {
        return errorCode >= ErrorType::LEGACY_ERROR && errorCode <= ErrorType::ERR_MAX;
    }

    /// return true if the given diagnostic can _only_ be an error; false otherwise
    bool isError(std::string_view name) {
        cstring lookup(name);
        // Some diagnostics might be both errors and warning/info
        // (e.g. "invalid" -> both ERR_INVALID and WARN_INVALID).
        bool error = false;
        for (const auto &pair : errorCatalog) {
            if (pair.second == lookup) {
                if (!isError(pair.first)) return false;
                error = true;
            }
        }

        return error;
    }

    void initReporter(ErrorReporter &reporter);

 private:
    ErrorCatalog() {}

    /// map from errorCode to name
    static std::map<int, cstring> errorCatalog;
};

}  // namespace P4

#endif /* LIB_ERROR_CATALOG_H_ */
