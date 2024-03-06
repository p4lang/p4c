#include "backends/p4tools/modules/smith/util/util.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#include <boost/random.hpp>

#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/util/wordlist.h"
#include "lib/cstring.h"

const std::vector<cstring> STR_KEYWORDS = {"if", "void", "else", "key", "actions", "true"};
static const std::array<char, 53> ALPHANUM = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"};

namespace P4Tools::P4Smith {

static boost::random::mt19937 rng;

void setSeed(int64_t seed) { rng = boost::mt19937(seed); }

int64_t getRndInt(int64_t min, int64_t max) {
    boost::random::uniform_int_distribution<int64_t> distribution(min, max);
    return distribution(rng);
}

big_int getRndBigInt(big_int min, big_int max) {
    boost::random::uniform_int_distribution<big_int> distribution(min, max);
    return distribution(rng);
}

std::string randStr(size_t len) {
    std::string ret;

    while (true) {
        std::stringstream ss;
        // Try to get a name from the wordlist.
        ss << Wordlist::getFromWordlist();
        size_t len_from_wordlist = ss.str().length();

        if (len_from_wordlist == len) {
            ret = ss.str();
        } else if (len_from_wordlist > len) {
            // We got a bigger word from the wordlist, so we have to truncate it.
            ret = ss.str().substr(0, len);
        } else if (len_from_wordlist < len) {
            // The word was too small so we append it.
            // Note: This also covers the case that we ran
            // out of the words from the wordlist.
            for (size_t i = len_from_wordlist; i < len; i++) {
                ss << ALPHANUM.at(getRndInt(0, sizeof(ALPHANUM) - 2));
            }
            ret = ss.str();
        }

        if (std::find(STR_KEYWORDS.begin(), STR_KEYWORDS.end(), ret) != STR_KEYWORDS.end()) {
            continue;
        }

        // The name is usable, break the loop.
        if (P4Scope::used_names.count(ret) == 0) {
            break;
        }
    }

    P4Scope::used_names.insert(ret);
    return ret;
}

int64_t randInt(const std::vector<int64_t> &percent) {
    int sum = accumulate(percent.begin(), percent.end(), 0);

    // Do not pick zero since that conflicts with zero percentage values.
    auto rand_num = getRndInt(1, sum);
    int ret = 0;

    int64_t ret_sum = 0;
    for (auto i : percent) {
        ret_sum += i;
        if (ret_sum >= rand_num) {
            break;
        }
        ret = ret + 1;
    }
    return ret;
}
}  // namespace P4Tools::P4Smith
