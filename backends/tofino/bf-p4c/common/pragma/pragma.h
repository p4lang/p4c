/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef EXTENSIONS_BF_P4C_COMMON_PRAGMA_PRAGMA_H_
#define EXTENSIONS_BF_P4C_COMMON_PRAGMA_PRAGMA_H_
#include <cstring>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <set>
#include <string>

namespace BFN {

/** Helper class for pragma help
 * The class captures a name, description, and detailed help for a pragma.
 * The class also stores to static lists for supported pragmas (external and internal).
 * The ParseAnnotation class creates instances of the Pragma class and registers them.
 *
 * The original intent was that each class will derive from this class, however,
 * registration needs an instance of the class, and implementing the support this way
 * only requires defining the three strings that provide the help.
 */
class Pragma {
 public:
    Pragma(const char *name, const char *description, const char *help) :
        _name(name), _description(description), _help(help) {}

    static void registerPragma(const Pragma *p, bool internal = false) {
        if (!internal)
            _supported_pragmas.insert(p);
        else
            _internal_pragmas.insert(p);
    }

    /// print the currently supported pragmas
    static std::ostream &printHelp(std::ostream &o) {
        // format a string to width
        auto format = [](const char *str, size_t width /*, size_t indent = 0 */) {
            std::string *res = new std::string("");
            do {
                std::string chunk = std::string(str).substr(0, width);
                auto newline = chunk.find('\n');
                if (newline != std::string::npos) {
                    *res += chunk.substr(0, newline+1);
                    str += newline+1;
                } else if (strlen(str) < width) {
                    *res += str;
                    break;  // no need to go further
                } else {
                    // split at the last space before the desired width
                    auto lastspace = chunk.rfind(' ');
                    if (lastspace != std::string::npos) {
                        *res += chunk.substr(0, lastspace) + "\n";
                        str += lastspace+1;
                    } else {
                        // no space? unlikely, so then we just print the string
                        *res += chunk + "\n";
                        str += chunk.size();
                    }
                }
            } while (strlen(str) > 0);
            return res;
        };

        auto formatPragma = [format](const Pragma *p, std::ostream &o) {
            o << std::endl << std::string(80, '-') << std::endl;
            auto d = format(p->_description, 80 - (strlen(p->_name) + 3));
            auto h = format(p->_help, 80);
            o << p->_name << ": " << d->c_str() << std::endl << std::endl
              << h->c_str() << std::endl;
            delete d;
            delete h;
        };

        o << "Supported pragmas:" << std::endl;
        for (auto &p : _supported_pragmas) formatPragma(p, o);
#if BAREFOOT_INTERNAL
        o << std::endl << std::string(80, '*') << std::endl
          << "Barefoot internal pragmas" << std::endl;
        for (auto &p : _internal_pragmas) formatPragma(p, o);
#endif
        return o;
    }

 private:
    const char *_name;
    const char *_description;
    const char *_help;

    struct PragmaCmp {
        bool operator()(const Pragma *a, const Pragma *b) const {
            return std::string(a->_name) < std::string(b->_name); }};

    /// set of externally supported pragmas
    static std::set<const Pragma *, PragmaCmp> _supported_pragmas;
    /// set of internally supported pragmas
    static std::set<const Pragma *, PragmaCmp> _internal_pragmas;
};

}  // end namespace BFN

#endif  // EXTENSIONS_BF_P4C_COMMON_PRAGMA_PRAGMA_H_
