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
    static const std::vector<std::string> P4_KEYWORDS = {"if",      "void", "else", "key",
                                                         "actions", "true", "for",  "in"};
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
    // Generate a random string of length 1 (i.e., a single character variable name, such as
    // i, j, k, etc.).
    std::string lowerCaseAlphabet = "abcdefghijklmnopqrstuvwxyz";
    int randomIndex = Utils::getRandInt(0, 25);

    // Debugging print
    std::cout << "Size of lowerCaseAlphabet: " << std::to_string(lowerCaseAlphabet.size()) << "\n";
    std::cout << "Random index: " << std::to_string(randomIndex) << "\n";

    char randomChar = lowerCaseAlphabet[randomIndex];
    return std::string(1, randomChar);
}

}  // namespace P4Tools::P4Smith
