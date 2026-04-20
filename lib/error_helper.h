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
#ifndef LIB_ERROR_HELPER_H_
#define LIB_ERROR_HELPER_H_

#include <type_traits>
#include <utility>

#include "lib/boost_format_helper.h"
#include "lib/error_message.h"
#include "lib/source_file.h"
#include "lib/stringify.h"

namespace P4 {
namespace priv {

inline void collectSourceInfo(ErrorMessage & /*msg*/) {}

template <typename T, typename... Rest>
void collectSourceInfo(ErrorMessage &msg, T &&arg, Rest &&...rest) {
    using RawType = std::remove_reference_t<T>;
    using ArgType = std::decay_t<T>;
    if constexpr (Util::has_SourceInfo_v<ArgType>) {
        if constexpr (std::is_convertible_v<ArgType, Util::SourceInfo>) {
            Util::SourceInfo info(arg);
            if (info.isValid()) {
                msg.locations.push_back(info);
            }
        } else {
            auto info = arg.getSourceInfo();
            if (info.isValid()) {
                msg.locations.push_back(info);
            }
        }
    } else if constexpr (std::is_array_v<RawType> &&
                         std::is_same_v<std::remove_cv_t<std::remove_extent_t<RawType>>, char>) {
        // String literal or char array: not a nullable pointer source.
    } else if constexpr (std::is_pointer_v<ArgType>) {
        using PointeeType = std::remove_pointer_t<ArgType>;
        if constexpr (Util::has_SourceInfo_v<PointeeType>) {
            if (arg != nullptr) {
                auto info = arg->getSourceInfo();
                if (info.isValid()) {
                    msg.locations.push_back(info);
                }
            }
        }
    } else if constexpr (std::is_same_v<ArgType, P4::Util::SourceInfo>) {
        if (arg.isValid()) {
            msg.locations.push_back(arg);
        }
    }
    collectSourceInfo(msg, std::forward<Rest>(rest)...);
}

}  // namespace priv

// Most direct invocations of error_helper usually only reduce arguments
template <class... Args>
ErrorMessage error_helper(const char *format, Args &&...args) {
    ErrorMessage msg;
    return error_helper(format, std::move(msg), std::forward<Args>(args)...);
}

// Invoked from ErrorReporter
template <class... Args>
ErrorMessage error_helper(const char *format, ErrorMessage msg, Args &&...args) {
    priv::collectSourceInfo(msg, args...);
    msg.message = createFormattedMessage(format, std::forward<Args>(args)...);
    return msg;
}

// This overload exists for backwards compatibility
template <class... Args>
ErrorMessage error_helper(const char *format, const std::string &prefix,
                          const Util::SourceInfo &info, const std::string &suffix, Args &&...args) {
    return error_helper(format, ErrorMessage(prefix, info, suffix), std::forward<Args>(args)...);
}

}  // namespace P4

#endif /* LIB_ERROR_HELPER_H_ */
