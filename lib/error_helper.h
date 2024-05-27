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

#include <boost/format.hpp>

#include "lib/error_message.h"
#include "lib/source_file.h"
#include "lib/stringify.h"

namespace priv {

// All these methods return std::string because this is the native format of boost::format
// Position is printed at the beginning.
static inline ErrorMessage error_helper(boost::format &f, ErrorMessage out) {
    out.message = boost::str(f);
    return out;
}

template <typename T, class... Args>
auto error_helper(boost::format &f, ErrorMessage out, const T *t, Args &&...args) {
    return error_helper(f, out, *t, std::forward<Args>(args)...);
}

template <class... Args>
ErrorMessage error_helper(boost::format &f, ErrorMessage out, const Util::SourceInfo &info,
                          Args &&...args) {
    if (info.isValid()) out.locations.push_back(info);
    return error_helper(f % "", std::move(out), std::forward<Args>(args)...);
}

template <typename T>
void maybeAddSourceInfo(ErrorMessage &out, const T &t) {
    if constexpr (Util::has_SourceInfo_v<T>) {
        auto info = t.getSourceInfo();
        if (info.isValid()) out.locations.push_back(info);
    }
}

template <typename T, class... Args>
auto error_helper(boost::format &f, ErrorMessage out, const T &t, Args &&...args)
    -> std::enable_if_t<!Util::has_toString_v<T> && !std::is_pointer_v<T>, ErrorMessage> {
    maybeAddSourceInfo(out, t);
    return error_helper(f % t, std::move(out), std::forward<Args>(args)...);
}

template <typename T, class... Args>
auto error_helper(boost::format &f, ErrorMessage out, const T &t,
                  Args &&...args) -> std::enable_if_t<Util::has_toString_v<T>, ErrorMessage> {
    maybeAddSourceInfo(out, t);
    return error_helper(f % t.toString(), std::move(out), std::forward<Args>(args)...);
}

}  // namespace priv

// Most direct invocations of error_helper usually only reduce arguments
template <class... Args>
ErrorMessage error_helper(boost::format &f, Args &&...args) {
    ErrorMessage msg;
    return ::priv::error_helper(f, msg, std::forward<Args>(args)...);
}

// Invoked from ErrorReporter
template <class... Args>
ErrorMessage error_helper(boost::format &f, ErrorMessage msg, Args &&...args) {
    return ::priv::error_helper(f, std::move(msg), std::forward<Args>(args)...);
}

// This overload exists for backwards compatibility
template <class... Args>
ErrorMessage error_helper(boost::format &f, const std::string &prefix, const Util::SourceInfo &info,
                          const std::string &suffix, Args &&...args) {
    return ::priv::error_helper(f, ErrorMessage(prefix, info, suffix), std::forward<Args>(args)...);
}

#endif /* LIB_ERROR_HELPER_H_ */
