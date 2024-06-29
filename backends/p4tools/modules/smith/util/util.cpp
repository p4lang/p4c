#include "backends/p4tools/modules/smith/util/util.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/util/wordlist.h"

namespace P4Tools::P4Smith {

std::string getRandomString(size_t len) {
    // Add "for" and "in" to the list of P4 keywords, even though the wordlist doesn't contain them.
    static const std::vector<std::string> P4_KEYWORDS = {"if",  "void",    "else",
                                                         "key", "actions", "true", "for", "in"};  
    static const std::array<char, 53> ALPHANUMERIC_CHARACTERS = {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"};

    std::string ret;

    while (true) {
        std::stringstream ss;
        // Try to get a name from the wordlist.
        ss << Wordlist::getFromWordlist();
        size_t lenFromWordlist = ss.str().length();

        if (lenFromWordlist == len) {
            ret = ss.str();
        } else if (lenFromWordlist > len) {
            // We got a bigger word from the wordlist, so we have to truncate it.
            ret = ss.str().substr(0, len);
        } else if (lenFromWordlist < len) {
            // The word was too small so we append it.
            // Note: This also covers the case that we ran
            // out of the words from the wordlist.
            for (size_t i = lenFromWordlist; i < len; i++) {
                ss << ALPHANUMERIC_CHARACTERS.at(
                    Utils::getRandInt(0, sizeof(ALPHANUMERIC_CHARACTERS) - 2));
            }
            ret = ss.str();
        }

        if (std::find(P4_KEYWORDS.begin(), P4_KEYWORDS.end(), ret) != P4_KEYWORDS.end()) {
            continue;
        }

        // The name is usable, break the loop.
        if (P4Scope::usedNames.count(ret) == 0) {
            break;
        }
    }

    P4Scope::usedNames.insert(ret);
    return ret;
}

/// Generate an arbitrary variable name for loop control.
std::string generateLoopControlVariable() {
    // Generate a random string of length 1 (i.e., a single character variable name, 
    // such as i, j, k, etc.).
    return getRandomString(1); 
}

/// TODO(zzmic): Consider deprecating the following function.
/// Generate a random loop initialization statement.
std::string generateForLoopInitialization(const std::string &var) {
    std::stringstream ss;
    int basicTypeOption = Utils::getRandInt(0, 3);     
    switch (basicTypeOption) {
        // Unsigned integer (bitstring) of size n.
        case 0:
            int bitFieldWidth = Utils::getRandInt(1, 64);
            ss << "bit<" << std::to_string(bitFieldWidth) << "> " << var << " = 0";
        // bit is the same as bit<1>.
        case 1:
            ss << "bit " << var << " = 0"; 
        // Signed integer (bitstring) of size n (>= 2). 
        case 2:
            int bitFieldWidth = Utils::getRandInt(2, 32);
            ss << "int<" << std::to_string(bitFieldWidth) << "> " << var << " = 0";
        // Variable-length bitstring.
        case 3:
            int bitFieldWidth = Utils::getRandInt(1, 64);
            ss << "varbit<" << std::to_string(bitFieldWidth) << "> " << var << " = 0";
    }
    return ss.str();
}

/// TODO(zzmic): Consider deprecating the following function.
/// Generate a random loop condition statement.
std::string generateForLoopCondition(const std::string &var) {
    std::stringstream ss;
    int upperBound = Utils::getRandInt(0, 100);
    ss << var << " < " << std::to_string(upperBound);
    return ss.str();
}

/// TODO(zzmic): Consider deprecating the following function.
/// Generate a random loop update (only consider increments for now) statement.
std::string generateForLoopUpdate(const std::string &var) {
    std::stringstream ss;
    int stepSize = Utils::getRandInt(0, 100);
    ss << var << " = " << var << " + " << std::to_string(stepSize);
    return ss.str();
}

/// TODO(zzmic): Consider deprecating the following function.
/// Generate a random loop progression statement (for for-in loop generation).
std::string generateForInLoopProgression(const std::string &var, const std::string &start, const std::string &end) {
    std::stringstream ss;
    int basicTypeOption = Utils::getRandInt(0, 3);
    switch (basicTypeOption) {
        // Unsigned integer (bitstring) of size n.
        case 0:
            int bitFieldWidth = Utils::getRandInt(1, 64);
            ss << "bit<" << std::to_string(bitFieldWidth) << ">" << var;
        // bit is the same as bit<1>.
        case 1:
            ss << "bit " << var; 
        // Signed integer (bitstring) of size n (>= 2). 
        case 2:
            int bitFieldWidth = Utils::getRandInt(2, 32);
            ss << "int<" << std::to_string(bitFieldWidth) << "> " << var;
        // Variable-length bitstring.
        case 3:
            int bitFieldWidth = Utils::getRandInt(1, 64);
            ss << "varbit<" << std::to_string(bitFieldWidth) << "> " << var;
    }
    ss << " in " << start << " .. " << end;
    return ss.str();
}

/// TODO(zzmic): Consider deprecating the following function.
/// Generate a random loop body that involves one variable.
std::string generateLoopBody(const std::string &var) {
    return "";
}

/// TODO(zzmic): Consider deprecating the following function.
/// Generate a random loop body that involves two variables.
std::string generateLoopBody(const std::string &var1, const std::string &var2) {    
    return "";
}

/// TODO(zzmic): Consider deprecating the following function.
/// Generate a random loop body that involves multiple (two or more but undetermined) variables.
template <typename... Args>
std::string generateLoopBody(const std::string &var1, const std::string &var2, const Args&... args) {
    return "";
}

}  // namespace P4Tools::P4Smith
