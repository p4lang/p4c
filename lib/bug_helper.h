/*
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

/* -*-c++-*- */

#ifndef LIB_BUG_HELPER_H_
#define LIB_BUG_HELPER_H_

#include <string>
#include <string_view>

#include <boost/format.hpp>

#include "absl/strings/str_cat.h"
#include "lib/cstring.h"
#include "lib/source_file.h"

template <class... Args>
std::string bug_helper(boost::format &f, std::string_view message, std::string_view position,
                       std::string_view tail, const char *t, Args &&...args);

template <class... Args>
std::string bug_helper(boost::format &f, std::string_view message, std::string_view position,
                       std::string_view tail, const cstring &t, Args &&...args);

template <class... Args>
std::string bug_helper(boost::format &f, std::string_view message, std::string_view position,
                       std::string_view tail, const Util::SourceInfo &info, Args &&...args);

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view message, std::string_view position,
                std::string_view tail, const T &t, Args &&...args) ->
    typename std::enable_if_t<std::is_base_of_v<Util::IHasSourceInfo, T>, std::string>;

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view message, std::string_view position,
                std::string_view tail, const T &t, Args &&...args) ->
    typename std::enable_if_t<!std::is_base_of_v<Util::IHasSourceInfo, T>, std::string>;

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view message, std::string_view position,
                std::string_view tail, const T *t, Args &&...args) ->
    typename std::enable_if_t<std::is_base_of_v<Util::IHasSourceInfo, T>, std::string>;

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view message, std::string_view position,
                std::string_view tail, const T *t, Args &&...args) ->
    typename std::enable_if_t<!std::is_base_of_v<Util::IHasSourceInfo, T>, std::string>;

// actual implementations
namespace detail {
static inline auto getPositionTail(const Util::SourceInfo &info, std::string_view position,
                                   std::string_view tail) {
    std::string_view posString = info.toPositionString().string_view();
    std::string outTail(tail);
    if (position.empty()) {
        position = posString;
    } else {
        outTail.append(posString);
        if (!posString.empty()) outTail.append("\n");
    }
    outTail += info.toSourceFragment();

    return std::make_pair(position, outTail);
}
}  // namespace detail

static inline std::string bug_helper(boost::format &f, std::string_view message,
                                     std::string_view position, std::string_view tail) {
    std::string text = boost::str(f);
    return absl::StrCat(position, position.empty() ? "" : ": ", message, text, "\n", tail);
}

template <class... Args>
std::string bug_helper(boost::format &f, std::string_view message, std::string_view position,
                       std::string_view tail, const char *t, Args &&...args) {
    return bug_helper(f % t, message, position, tail, std::forward<Args>(args)...);
}

template <class... Args>
std::string bug_helper(boost::format &f, std::string_view message, std::string_view position,
                       std::string_view tail, const cstring &t, Args &&...args) {
    return bug_helper(f % t.string_view(), message, position, tail, std::forward<Args>(args)...);
}

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view message, std::string_view position,
                std::string_view tail, const T *t, Args &&...args) ->
    typename std::enable_if_t<!std::is_base_of_v<Util::IHasSourceInfo, T>, std::string> {
    std::stringstream str;
    str << t;
    return bug_helper(f % str.str(), message, position, tail, std::forward<Args>(args)...);
}

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view message, std::string_view position,
                std::string_view tail, const T &t, Args &&...args) ->
    typename std::enable_if_t<!std::is_base_of_v<Util::IHasSourceInfo, T>, std::string> {
    return bug_helper(f % t, message, position, tail, std::forward<Args>(args)...);
}

template <class... Args>
std::string bug_helper(boost::format &f, std::string_view message, std::string_view position,
                       std::string_view tail, const Util::SourceInfo &info, Args &&...args) {
    auto [outPos, outTail] = detail::getPositionTail(info, position, tail);
    return bug_helper(f % "", message, outPos, outTail, std::forward<Args>(args)...);
}

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view message, std::string_view position,
                std::string_view tail, const T *t, Args &&...args) ->
    typename std::enable_if_t<std::is_base_of_v<Util::IHasSourceInfo, T>, std::string> {
    if (t == nullptr) return bug_helper(f, message, position, tail, std::forward<Args>(args)...);

    auto [outPos, outTail] = detail::getPositionTail(t->getSourceInfo(), position, tail);
    std::stringstream str;
    str << t;
    return bug_helper(f % str.str(), message, outPos, outTail, std::forward<Args>(args)...);
}

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view message, std::string_view position,
                std::string_view tail, const T &t, Args &&...args) ->
    typename std::enable_if_t<std::is_base_of_v<Util::IHasSourceInfo, T>, std::string> {
    auto [outPos, outTail] = detail::getPositionTail(t.getSourceInfo(), position, tail);

    std::stringstream str;
    str << t;
    return bug_helper(f % str.str(), message, outPos, outTail, std::forward<Args>(args)...);
}

#endif /* LIB_BUG_HELPER_H_ */
