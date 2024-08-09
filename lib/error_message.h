/*
Copyright 2013-present Barefoot Networks, Inc.

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
#ifndef LIB_ERROR_MESSAGE_H_
#define LIB_ERROR_MESSAGE_H_

#include <cstring>
#include <vector>

#include "lib/source_file.h"

namespace P4 {

/**
 *  Structure populated via error_helper functions
 *
 *  Typically, calls to ::P4::error/::P4::warning have many parameters, some of them
 *  might have SourceInfo attribute. ::P4::error_helper parse those parameters, format
 *  parameters to output message and extracts SourceInfo wherever possible.
 *
 *  Populated structure can be serialized to canonical error message with toString() method.
 *
 *  This structure is mainly used inside ErrorReporter, but some uses invoke ::P4::error_helper
 *  directly and those uses need to call toString() on returned object.
 */
struct ErrorMessage {
    enum class MessageType : std::size_t { None, Error, Warning, Info };

    MessageType type = MessageType::None;
    std::string prefix;                            /// Typically error/warning type from catalog
    std::string message;                           /// Particular formatted message
    std::vector<Util::SourceInfo> locations = {};  /// Relevant source locations for this error
    std::string suffix;                            /// Used by errorWithSuffix

    ErrorMessage() {}
    // Invoked from backwards compatible error_helper
    ErrorMessage(std::string prefix, Util::SourceInfo info, std::string suffix)
        : prefix(std::move(prefix)), locations({info}), suffix(std::move(suffix)) {}
    // Invoked from error_reporter
    ErrorMessage(MessageType type, std::string prefix, std::string suffix)
        : type(type), prefix(std::move(prefix)), suffix(std::move(suffix)) {}
    ErrorMessage(MessageType type, std::string prefix, std::string message,
                 const std::vector<Util::SourceInfo> &locations, std::string suffix)
        : type(type),
          prefix(std::move(prefix)),
          message(std::move(message)),
          locations(locations),
          suffix(std::move(suffix)) {}

    std::string getPrefix() const;
    std::string toString() const;
};

/**
 *  Variation on ErrorMessage, this one is used for errors coming from parser.
 *  This is exclusively used in ErrorReporter
 */
struct ParserErrorMessage {
    Util::SourceInfo location;
    std::string message;

    ParserErrorMessage(Util::SourceInfo loc, std::string msg)
        : location(loc), message(std::move(msg)) {}

    std::string toString() const;
};

}  // namespace P4

#endif /* LIB_ERROR_MESSAGE_H_ */
