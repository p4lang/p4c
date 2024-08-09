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

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <boost/format.hpp>

#include "absl/strings/str_cat.h"
#include "cstring.h"
#include "source_file.h"
#include "stringify.h"

namespace P4 {
namespace detail {

static inline std::pair<std::string_view, std::string> getPositionTail(const Util::SourceInfo &info,
                                                                       std::string_view position,
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

    return std::pair(position, outTail);
}

template <typename T>
std::pair<std::string_view, std::string> maybeAddSourceInfo(const T &t, std::string_view position,
                                                            std::string_view tail) {
    if constexpr (Util::has_SourceInfo_v<T>)
        return getPositionTail(t.getSourceInfo(), position, tail);

    (void)position;
    (void)tail;
    return {"", ""};
}

static inline std::string bug_helper(boost::format &f, std::string_view position,
                                     std::string_view tail) {
    return absl::StrCat(position, position.empty() ? "" : ": ", boost::str(f), "\n", tail);
}

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view position, std::string_view tail, const T *t,
                Args &&...args);

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view position, std::string_view tail, const T &t,
                Args &&...args) -> std::enable_if_t<!std::is_pointer_v<T>, std::string>;

template <class... Args>
std::string bug_helper(boost::format &f, std::string_view position, std::string_view tail,
                       const char *t, Args &&...args) {
    return bug_helper(f % t, position, tail, std::forward<Args>(args)...);
}

template <class... Args>
std::string bug_helper(boost::format &f, std::string_view position, std::string_view tail,
                       const Util::SourceInfo &info, Args &&...args) {
    auto [outPos, outTail] = detail::getPositionTail(info, position, tail);
    return bug_helper(f % "", outPos, outTail, std::forward<Args>(args)...);
}

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view position, std::string_view tail, const T *t,
                Args &&...args) {
    if (t == nullptr) return bug_helper(f, position, tail, std::forward<Args>(args)...);

    auto [outPos, outTail] = maybeAddSourceInfo(*t, position, tail);
    // We define operator<< for all T's that have either dbprint() or toString()
    // methods (in stringify.h).  Note, however, that these overloads are
    // provided in the global namespace, not namespace of the T itself and
    // therefore cannot be found via ADL. As a result, name lookup for the `str
    // << t` below succeeds as it is encloding namespace for `detail`. However,
    // `f % t` will fail as operator<< will be called from some
    // `boost::io::detail` namespace for which global namespace is not a direct
    // enclosing one and therefore the lookup for operator<< will fail.
    std::stringstream str;
    str << t;
    return bug_helper(f % str.str(), outPos, outTail, std::forward<Args>(args)...);
}

template <typename T, class... Args>
auto bug_helper(boost::format &f, std::string_view position, std::string_view tail, const T &t,
                Args &&...args) -> std::enable_if_t<!std::is_pointer_v<T>, std::string> {
    auto [outPos, outTail] = maybeAddSourceInfo(t, position, tail);
    std::stringstream str;
    str << t;
    return bug_helper(f % str.str(), outPos, outTail, std::forward<Args>(args)...);
}
}  // namespace detail

// Most direct invocations of bug_helper usually only reduce arguments
template <class... Args>
std::string bug_helper(boost::format &f, std::string_view position, std::string_view tail,
                       Args &&...args) {
    return detail::bug_helper(f, position, tail, std::forward<Args>(args)...);
}

}  // namespace P4

#endif /* LIB_BUG_HELPER_H_ */
