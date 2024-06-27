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
    // Add "for" and "in" to the list of P4 keywords.
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

/// Generate a random loop initialization statement.
std::string generateLoopInitialization(const std::string &var) {
    std::stringstream ss;

    // TOOD: Add support for other types of initialization, 
    // although its quite common to initialize a loop control variable to 0.
    int bitFieldWidth = Utils::getRandInt(1, 64);
    ss << "bit<" << std::to_string(bitFieldWidth) << "> " << var << " = 0";
    
    return ss.str();
}

/// Generate a random loop condition statement.
std::string generateLoopCondition(const std::string &var) {
    std::stringstream ss;

    // TODO(zzmic): Add support for other types of conditions.
    int upperBound = Utils::getRandInt(0, 100);
    ss << var << " < " << std::to_string(upperBound);

    return ss.str();
}

/// Generate a random loop update (only consider increments for now) statement.
std::string generateLoopUpdate(const std::string &var) {
    std::stringstream ss;

    // TOOD: Add support for other types of updates.
    int stepSize = Utils::getRandInt(0, 100);
    ss << var << " = " << var << " + " << std::to_string(stepSize);

    return ss.str();
}

/// Generate a random loop body that involves one variable.
std::string generateLoopBody(const std::string &var) {
    // TODO(zzmic): Add actual support for the loop body.
    return "";
}

/// Generate a random loop body that involves two variables.
std::string generateLoopBody(const std::string &var1, const std::string &var2) {
    // TODO(zzmic): Add actual support for the loop body.
    return "";
}

/// Generate a random loop body that involves multiple (two or more but undetermined) variables.
/// TODO(zzmic): Need to figure out whether this is permissible/advisable.
template <typename... Args>
std::string generateLoopBody(const std::string &var1, const std::string &var2, const Args&... args) {
    // TODO(zzmic): Add actual support for the loop body.
    return "";
}

/// Generate a trivial but complete for loop (traditional C-style for-loop).
/// The generated for-loop will be similar to `testdata/p4_16_samples/forloop5.p4` (except the loop body).
std::string generateTrivialForLoop() {
    std::string loopControlVar = generateLoopControlVariable();
    std::string loopInit = generateLoopInitialization(loopControlVar);
    std::string loopCondition = generateLoopCondition(loopControlVar);
    std::string loopUpdate = generateLoopUpdate(loopControlVar);
    std::string loopBody = generateLoopBody(loopControlVar);

    std::stringstream ss;
    ss << "for (" << loopInit << "; " << loopCondition << "; " << loopUpdate << ") {\n";
    ss << loopBody << "\n";
    ss << "}";
    return ss.str();
}

/// Generate a for-loop iterating over a header stack. 
// The generated for-loop will be similar to the first for-loop in `testdata/p4_16_samples/forloop1.p4` (except the loop body).
std::string generateHeaderStackForLoop() {
    std::string varType = "t2";
    std::string loopVar = generateLoopControlVariable();
    std::string stackName = "hdrs.stack";
    std::string loopBody = generateLoopBody(loopVar);

    std::stringstream ss;
    ss << "for (" << varType << " " << loopVar << " in " << stackName << ") {\n";
    ss << loopBody << "\n";
    ss << "}";
    return ss.str();
}

/// Generate a for-loop with multiple initialization and update statements.
/// The generated for-loop will be similar to the second for-loop in `testdata/p4_16_samples/forloop1.p4` (except the loop body).
std::string generateMultipleInitUpdateForLoop() {
    std::string loopVar1 = generateLoopControlVariable();
    std::string loopVar2 = generateLoopControlVariable();
    int bitFieldWidth = Utils::getRandInt(1, 64);
    std::string loopInit = "bit<" + std::to_string(bitFieldWidth) + "> " + loopVar1 + " = 0, " + loopVar2 + " = 1";
    std::string loopCondition = loopVar1 + " < hdrs.head.cnt";
    std::string loopUpdate = loopVar1 + " = " + loopVar1 + " + 1, " + loopVar2 + " = " + loopVar2 + " << 1";
    std::string loopBody = generateLoopBody(loopVar1, loopVar2);

    std::stringstream ss;
    ss << "for (" << loopInit << "; " << loopCondition << "; " << loopUpdate << ") {\n";
    ss << loopBody << "\n";
    ss << "}";
    return ss.str();
}

/// Generate a range-based for-loop.
std::string generateRangeBasedForLoop(const std::string &start, const std::string &end) {
    std::string loopVar = generateLoopControlVariable();
    int bitFieldWidth = Utils::getRandInt(1, 64);
    
    std::stringstream ss;
    ss << "for (bit<" + std::to_string(bitFieldWidth) + "> " << loopVar << " in " << start << " .. " << end << ") {\n";
    ss << generateLoopBody(loopVar) << "\n";
    ss << "}";
    return ss.str();
}

/// Main/major function to generate a for-loop.
std::string generateForLoop() {
    // TODO(zzmic): Increment the range of the random number generator if more types of for-loop generations are added,
    int choice = Utils::getRandInt(0, 3);

    switch (choice) {
        case 0:
            return generateTrivialForLoop();
        case 1:
            return generateHeaderStackForLoop();
        case 2:
            return generateMultipleInitUpdateForLoop();
        case 3:
            return generateRangeBasedForLoop("0", std::to_string(Utils::getRandInt(0, 100)));
        default:
            return generateTrivialForLoop();
    }
}

}  // namespace P4Tools::P4Smith
