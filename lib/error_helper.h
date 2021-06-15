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
#ifndef _LIB_ERROR_HELPER_H_
#define _LIB_ERROR_HELPER_H_

#include <stdarg.h>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>

#include <boost/format.hpp>

#include "lib/cstring.h"
#include "lib/source_file.h"
#include "lib/stringify.h"

struct ErrorMessage {
    enum class Type : std::size_t { None, Error, Warning };

    Type type = Type::None;
    std::string prefix = "";  // Typically error/warning type from catalog
    std::string message = "";  // Actual formatted message
    std::vector<Util::SourceInfo> locations = {}; // Relevant source locations for this error
    std::string suffix = "";  // Used by errorWithSuffix

/*
    Util::SourceInfo position;  // Position in P4 source
    std::string tail = "";  // Extra P4 source positions
*/

    ErrorMessage() {}
    // Invoked from backwards compatible error_helper
    ErrorMessage(const std::string &prefix, const Util::SourceInfo &info, std::string &suffix)
        : prefix(prefix), locations({info}), suffix(suffix) {}
    // Invoked from error_reporter
    ErrorMessage(Type type, const std::string &prefix, const std::string &suffix)
        : type(type), prefix(prefix), suffix(suffix) {}

    std::string getPrefix() const {
        std::string p = prefix;
        if (type == Type::Error) {
            if (p.empty()) p = "error: ";
            else p = "[--Werror=" + p + "] error: ";
        } else if (type == Type::Warning) {
            if (p.empty()) p = "warning: ";
            else p = "[--Wwarn=" + p + "] warning: ";
        }
        return p;
    }

    // original implementation, TODO: delete this
    /*std::string _toString() const {
        std::string result = position.toPositionString().c_str();
        if (!result.empty()) result += ": ";
        result += getPrefix() + message + "\n" + tail + suffix;
        return result;
    }*/

    std::string toString() const {
        std::string result = "";
        std::string mainFragment = "";
        if (!locations.empty() && locations.front().isValid()) {
            result += locations.front().toPositionString() + ": ";
            mainFragment = locations.front().toSourceFragment();
        }

        result += getPrefix() + message + "\n" + mainFragment;

        for (unsigned i = 1; i < locations.size(); i++) {
            if (locations[i].isValid())
                result += locations[i].toPositionString() + "\n" + locations[i].toSourceFragment();
        }

        result += suffix;

        return result;
    }
};

struct ParserErrorMessage {
    Util::SourceInfo location;
    cstring message;

    ParserErrorMessage(const Util::SourceInfo &loc, const cstring &msg) : location(loc), message(msg) {}

    std::string toString() const {
        return std::string(location.toPositionString().c_str()) + ":" + message + "\n" + location.toSourceFragment().c_str();
    }
};

namespace priv {

// All these methods return std::string because this is the native format of boost::format
// Position is printed at the beginning.
static inline ErrorMessage error_helper(boost::format& f, ErrorMessage out) {
    out.message = boost::str(f);
    return out;
}

template<class... Args>
ErrorMessage error_helper(boost::format& f, ErrorMessage out,
                         const char* t, Args... args);

template<class... Args>
ErrorMessage error_helper(boost::format& f, ErrorMessage out,
                         const cstring& t, Args... args);

// use: ir/mau.cpp:805
template<class... Args>
ErrorMessage error_helper(boost::format& f, ErrorMessage out,
                         const Util::SourceInfo &info, Args... args);

template<typename T, class... Args>
auto error_helper(boost::format& f, ErrorMessage out,
                  const T &t, Args... args) ->
    typename std::enable_if<Util::HasToString<T>::value &&
                            !std::is_base_of<Util::IHasSourceInfo, T>::value, ErrorMessage>::type;

template<typename T, class... Args>
auto error_helper(boost::format& f, ErrorMessage out,
                  const T *t, Args... args) ->
    typename std::enable_if<Util::HasToString<T>::value &&
                            !std::is_base_of<Util::IHasSourceInfo, T>::value, ErrorMessage>::type;

template<typename T, class... Args>
auto error_helper(boost::format& f, ErrorMessage out,
                  const T &t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, ErrorMessage>::type;

template<typename T, class... Args>
auto error_helper(boost::format& f, ErrorMessage out,
                  const T *t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, ErrorMessage>::type;

template<class... Args>
ErrorMessage error_helper(boost::format& f, ErrorMessage out,
                         const big_int *t, Args... args);

template<class... Args>
ErrorMessage error_helper(boost::format& f, ErrorMessage out,
                         const big_int &t, Args... args);

template<typename T, class... Args>
auto error_helper(boost::format& f, ErrorMessage out,
             const T& t, Args... args) ->
    typename std::enable_if<std::is_arithmetic<T>::value, ErrorMessage>::type;

// actual implementations

template<class... Args>
ErrorMessage error_helper(boost::format& f, ErrorMessage out,
                         const char* t, Args... args) {
    return error_helper(f % t, out, std::forward<Args>(args)...);
}

template<class... Args>
ErrorMessage error_helper(boost::format& f, ErrorMessage out,
                         const cstring& t, Args... args) {
    return error_helper(f % t.c_str(), out, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto error_helper(boost::format& f, ErrorMessage out,
                  const T &t, Args... args) ->
    typename std::enable_if<Util::HasToString<T>::value &&
                            !std::is_base_of<Util::IHasSourceInfo, T>::value, ErrorMessage>::type {
    return error_helper(f % t.toString(), out, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto error_helper(boost::format& f, ErrorMessage out,
                  const T *t, Args... args) ->
    typename std::enable_if<Util::HasToString<T>::value &&
                            !std::is_base_of<Util::IHasSourceInfo, T>::value, ErrorMessage>::type {
    return error_helper(f % t->toString(), out, std::forward<Args>(args)...);
}

template<class... Args>
ErrorMessage error_helper(boost::format& f, ErrorMessage out,
                         const big_int *t, Args... args) {
    return error_helper(f % t, out, std::forward<Args>(args)...);
}

template<class... Args>
ErrorMessage error_helper(boost::format& f, ErrorMessage out, const big_int &t, Args... args) {
    return error_helper(f % t, out, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto error_helper(boost::format& f, ErrorMessage out, const T& t, Args... args) ->
    typename std::enable_if<std::is_arithmetic<T>::value, ErrorMessage>::type {
    return error_helper(f % t, out, std::forward<Args>(args)...);
}

template<class... Args>
ErrorMessage error_helper(boost::format& f, ErrorMessage out, const Util::SourceInfo &info,
                         Args... args) {
    /*cstring posString = info.toPositionString();
    if (!out.position.isValid()) { // if there is not already something stored in out.position
        out.position = info;
        posString = "";
    } else {
        if (!posString.isNullOrEmpty())
            posString += "\n";
    }
    out.tail += posString + info.toSourceFragment();*/

    if (info.isValid()) out.locations.push_back(info);
    return error_helper(f % "", out, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto error_helper(boost::format& f, ErrorMessage out, const T *t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, ErrorMessage>::type {
    /*cstring posString = t->getSourceInfo().toPositionString();
    if (!out.position.isValid()) {
        out.position = t->getSourceInfo();
        posString = "";
    } else {
        if (!posString.isNullOrEmpty())
            posString += "\n";
    }
    out.tail += posString + t->getSourceInfo().toSourceFragment();*/

    auto info = t->getSourceInfo();
    if (info.isValid()) out.locations.push_back(info);
    return error_helper(f % t->toString(), out, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto error_helper(boost::format& f, ErrorMessage out, const T &t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, ErrorMessage>::type {
    /*cstring posString = t.getSourceInfo().toPositionString();
    if (!out.position.isValid()) {
        out.position = t.getSourceInfo();
        posString = "";
    } else {
        if (!posString.isNullOrEmpty())
            posString += "\n";
    }
    out.tail += posString + t.getSourceInfo().toSourceFragment();*/

    auto info = t.getSourceInfo();
    if (info.isValid()) out.locations.push_back(info);
    return error_helper(f % t.toString(), out, std::forward<Args>(args)...);
}

}  // namespace priv

// Most direct invocations of error_helper usually only reduce arguments
template<class... Args>
ErrorMessage error_helper(boost::format& f, Args... args) {
    ErrorMessage msg;
    return ::priv::error_helper(f, msg, std::forward<Args>(args)...);
}

// Invoked from ErrorReporter
template<class... Args>
ErrorMessage error_helper(boost::format& f, ErrorMessage &msg, Args... args) {
    return ::priv::error_helper(f, msg, std::forward<Args>(args)...);
}

// This overload exists for backwards compatibility
template<class... Args>
ErrorMessage error_helper(boost::format& f, const std::string &prefix,
                          const Util::SourceInfo &info, const std::string &suffix, Args... args) {
    ErrorMessage msg(prefix, info, suffix);
    return ::priv::error_helper(f, msg, std::forward<Args>(args)...);
}

/***********************************************************************************/

static inline std::string bug_helper(boost::format& f, std::string message,
                                     std::string position, std::string tail) {
    std::string text = boost::str(f);
    std::string result = position;
    if (!position.empty())
        result += ": ";
    result += message + text + "\n" + tail;
    return result;
}

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const char* t, Args... args);

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const cstring& t, Args... args);

template<class... Args>
std::string bug_helper(boost::format& f, std::string message,
                       std::string position, std::string tail,
                       const Util::SourceInfo &info, Args... args);

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T &t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type;

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T *t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type;

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T *t, Args... args) ->
    typename std::enable_if<!std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type;

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const big_int *t, Args... args);

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const big_int &t, Args... args);

template<typename T, class... Args>
auto
bug_helper(boost::format& f, std::string message, std::string position,
           std::string tail, const T& t, Args... args) ->
    typename std::enable_if<!std::is_base_of<Util::IHasSourceInfo, T>::value,
                            std::string>::type;

// actual implementations

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const char* t, Args... args) {
    return bug_helper(f % t, message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const cstring& t, Args... args) {
    return bug_helper(f % t.c_str(), message, position, tail, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T *t, Args... args) ->
        typename std::enable_if<!std::is_base_of<Util::IHasSourceInfo, T>::value,
                                std::string>::type {
    std::stringstream str;
    str << t;
    return bug_helper(f % str.str(), message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const big_int *t, Args... args) {
    return bug_helper(f % t, message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const big_int &t, Args... args) {
    return bug_helper(f % t, message, position, tail, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto
bug_helper(boost::format& f, std::string message, std::string position,
           std::string tail, const T& t, Args... args) ->
    typename std::enable_if<!std::is_base_of<Util::IHasSourceInfo, T>::value,
                            std::string>::type {
    return bug_helper(f % t, message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const Util::SourceInfo &info, Args... args) {
    cstring posString = info.toPositionString();
    if (position.empty()) {
        position = posString;
        posString = "";
    } else {
        if (!posString.isNullOrEmpty()) {
            posString += "\n";
        }
    }
    return bug_helper(f % "", message, position, tail + posString + info.toSourceFragment(),
                        std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T *t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type {
    if (t == nullptr) {
        return bug_helper(f, message, position,
                          tail, std::forward<Args>(args)...);
    }

    cstring posString = t->getSourceInfo().toPositionString();
    if (position.empty()) {
        position = posString;
        posString = "";
    } else {
        if (!posString.isNullOrEmpty()) {
            posString += "\n";
        }
    }
    std::stringstream str;
    str << t;
    return bug_helper(f % str.str(), message, position,
                      tail + posString + t->getSourceInfo().toSourceFragment(),
                      std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T &t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type {
    cstring posString = t.getSourceInfo().toPositionString();
    if (position.empty()) {
        position = posString;
        posString = "";
    } else {
        if (!posString.isNullOrEmpty()) {
            posString += "\n";
        }
    }
    std::stringstream str;
    str << t;
    return bug_helper(f % str.str(), message, position,
                        tail + posString + t.getSourceInfo().toSourceFragment(),
                        std::forward<Args>(args)...);
}

#endif   // _LIB_ERROR_HELPER_H_
