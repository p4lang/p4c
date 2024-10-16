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

#ifndef EXTENSIONS_BF_P4C_COMMON_PRAGMA_H_
#define EXTENSIONS_BF_P4C_COMMON_PRAGMA_H_
#include <cstring>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <set>
#include <string>

namespace BFN {

/** Abstract class/interface for pragmas */

class Pragma {
 public:
    /// the pragma name -- should be overriden
    virtual const char *name() const { return "undefined"; }

    /// short description that will be printed as help
    virtual const char *description() const = 0;

    /// detailed description that will be printed as help
    virtual const char *help() const = 0;

    /// print the currently supported pragmas
    static std::ostream &printHelp(std::ostream &o) {
        // format a string to width
        auto format = [](const char *str, size_t width, size_t indent = 0) -> const char * {
            std::string res = "";
            do {
                std::string chunk = std::string(str).substr(0, width);
                auto newline = chunk.find('\n');
                if (newline != std::string::npos) {
                    res += chunk.substr(0, newline+1);
                    str += newline+1;
                } else if (strlen(str) < width) {
                    res += str;
                    break;  // no need to go further
                } else {
                    // split at the last space before the desired width
                    auto lastspace = chunk.rfind(' ');
                    if (lastspace != std::string::npos) {
                        res += chunk.substr(0, lastspace) + "\n";
                        str += lastspace+1;
                    } else {
                        // no space? unlikely, so then we just print the string
                        res += chunk + "\n";
                        str += chunk.size();
                    }
                }
            } while (strlen(str) > 0);
            return res.c_str();
        };

        auto formatPragma = [format](const Pragma *p, std::ostream &o) {
            o << std::endl;
            o << p->name() << ": " << format(p->description(), 80 - (strlen(p->name()) + 3))
              << std::endl << std::endl;
            o << format(p->help(), 80) << std::endl;
        };

        o << "Supported pragmas:" << std::endl;
        for (auto &p : _supported_pragmas) formatPragma(p, o);
#if BAREFOOT_INTERNAL
        for (auto &p : _internal_pragmas) formatPragma(p, o);
#endif
        return o;
    }

 protected:
    /// add a pragma to the list of supported pragmas
    /// invoked by subclasses to register themselves
    static bool registerPragma(const Pragma *p, bool internal = false) {
        std::cerr << "registering " << p->name() << std::endl;
        if (!internal)
            _supported_pragmas.insert(p);
        else
            _internal_pragmas.insert(p);
        return true;
    }

 private:
    struct PragmaCmp {
        bool operator()(const Pragma *a, const Pragma *b) const {
            return std::string(a->name()) < std::string(b->name());
        }
    };

    /// set of externally supported pragmas
    static std::set<const Pragma *, PragmaCmp> _supported_pragmas;
    /// set of internally supported pragmas
    static std::set<const Pragma *, PragmaCmp> _internal_pragmas;
};

}  // end namespace BFN

#endif  // EXTENSIONS_BF_P4C_COMMON_PRAGMA_H_
