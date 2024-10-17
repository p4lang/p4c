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

#ifndef BF_P4C_PHV_PARSER_EXTRACT_BALANCE_SCORE_H_
#define BF_P4C_PHV_PARSER_EXTRACT_BALANCE_SCORE_H_

/* Note: this is an incorrect header. It contains function implementations, hence it can
 *    be included just in one translation unit. Otherwise, linking conflicts will
 *    happen. */

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

#include "bf-p4c/common/table_printer.h"
#include "bf-p4c/phv/phv.h"

struct StateExtractUsage {
    static const std::vector<PHV::Size> extractor_sizes;

    // TODO would be nice to get this from PardeSpec. JBay has one size,
    // 16-bit extractors, so the extractor balance may not even be relevant.

    std::map<PHV::Size, unsigned> use = { {PHV::Size::b8,  0},
                                          {PHV::Size::b16, 0},
                                          {PHV::Size::b32, 0} };

    StateExtractUsage() { }

    explicit StateExtractUsage(const std::set<PHV::Container>& containers) {
        for (auto c : containers)
            use[c.type().size()]++;
    }

    // Given two state extract usages of same number of extracted bytes, which is
    // better in terms of bandwidth utilization?
    //
    // This is to be used before PHV allocation, where we assume each state has
    // infinite bandwidth; After PHV allocation, big states are split in order to
    // satisfy each state's capacity constraint (4xB, 4xH, 4xW).
    //
    // The objective is to avoid spilling extracts into a next state whilst they
    // can be extracted upfront. This requires a balanced usage of the three extractor
    // sizes.
    //
    // We implemented the tie break based on the following heuristics:
    //    1. Compare the delta of max used and min used extractor size
    //    2. Compare the number of extract sizes that exceed 4
    //    3. Compare the total number of extracts
    //    4. Compare the number of extracts of each szie in descending order
    //
    // e.g. below is all possible combinations of 20 bytes of extraction, and associated
    // score (with 0 being the best score, and the more negative the worse).
    //
    //  -----------------------
    //  |   B|   H|  W|  Score|
    //  -----------------------
    //  |   2|   3|  3|      0|
    //  |   4|   2|  3|     -1|
    //  |   4|   4|  2|     -2|
    //  |   2|   1|  4|     -3|
    //  |   2|   5|  2|     -4|
    //  |   0|   2|  4|     -5|
    //  |   0|   4|  3|     -6|
    //  |   4|   0|  4|     -7|
    //  |   6|   3|  2|     -8|
    //  |   0|   0|  5|     -9|
    //  |   6|   1|  3|    -10|
    //  |   4|   6|  1|    -11|
    //  |   6|   5|  1|    -12|
    //  |   0|   6|  2|    -13|
    //  |   2|   7|  1|    -14|
    //  |   8|   2|  2|    -15|
    //  |   8|   4|  1|    -16|
    //  |   6|   7|  0|    -17|
    //  |   0|   8|  1|    -18|
    //  |   8|   0|  3|    -19|
    //  |   4|   8|  0|    -20|
    //  |   8|   6|  0|    -21|
    //  |   2|   9|  0|    -22|
    //  |  10|   1|  2|    -23|
    //  |  10|   3|  1|    -24|
    //  |   0|  10|  0|    -25|
    //  |  10|   5|  0|    -26|
    //  |  12|   2|  1|    -27|
    //  |  12|   0|  2|    -28|
    //  |  12|   4|  0|    -29|
    //  |  14|   1|  1|    -30|
    //  |  14|   3|  0|    -31|
    //  |  16|   0|  1|    -32|
    //  |  16|   2|  0|    -33|
    //  |  18|   1|  0|    -34|
    //  |  20|   0|  0|    -35|
    //  -----------------------
    //
    bool operator<(StateExtractUsage b) const {
        BUG_CHECK(use.size() == 3 && b.use.size() == 3, "malformed extractor use");

        if (total_bytes() < b.total_bytes()) return true;
        if (total_bytes() > b.total_bytes()) return false;

        auto a_sorted = sorted();
        auto b_sorted = b.sorted();

        unsigned a_delta = a_sorted[2] - a_sorted[0];
        unsigned b_delta = b_sorted[2] - b_sorted[0];

        if (a_delta < b_delta) return true;
        if (a_delta > b_delta) return false;

        unsigned a_over_four = 0;
        unsigned b_over_four = 0;

        for (auto u : use) {
            if (u.second > 4)
                a_over_four++;
        }

        for (auto u : b.use) {
            if (u.second > 4)
                b_over_four++;
        }

        if (a_over_four < b_over_four) return true;
        if (a_over_four > b_over_four) return false;

        if (total_extracts() < b.total_extracts()) return true;
        if (total_extracts() > b.total_extracts()) return false;

        for (int i = 2; i >= 0; i--) {
            if (a_sorted[i] > b_sorted[i]) return true;
            if (a_sorted[i] < b_sorted[i]) return false;
        }

        return false;
    }

    bool operator==(const StateExtractUsage& c) const {
        return use == c.use;
    }

    unsigned total_extracts() const {
        unsigned total = 0;
        for (auto sz : extractor_sizes)
            total += use.at(sz);

        return total;
    }

    unsigned total_bytes() const {
        unsigned total = 0;
        for (auto sz : extractor_sizes)
            total += (unsigned)sz/8 * use.at(sz);

        return total;
    }

    void print() const {
        for (auto sz : extractor_sizes)
            std::cout << use.at(sz) << " ";

        std::cout << std::endl;
    }

    std::vector<unsigned> sorted() const {
        std::vector<unsigned> rv;

        for (auto sz : extractor_sizes)
            rv.push_back(use.at(sz));

        std::sort(rv.begin(), rv.end());
        return rv;
    }
};

namespace ParserExtractScore {

void verify(const StateExtractUsage& use, unsigned num_bytes) {
    BUG_CHECK(use.total_bytes() == num_bytes, "number of bytes don't add up");
}

std::string
print_scoreboard(unsigned num_bytes, const std::set<StateExtractUsage>& combos) {
    std::stringstream ss;
    ss << "Scoreboard for " << num_bytes << " bytes:" << std::endl;

    TablePrinter tp(ss, {"B", "H", "W", "Score"});

    int score = 0;
    for (auto& use : combos) {
        tp.addRow({ std::to_string(use.use.at(PHV::Size::b8)),
                    std::to_string(use.use.at(PHV::Size::b16)),
                    std::to_string(use.use.at(PHV::Size::b32)),
                    std::to_string(score--)
                  });
    }

    tp.print();
    return ss.str();
}

// What are all possible combinations of extracts (B, H, W) that add up to N bytes?
// This is a textbook dynamic programming problem (memoization + optimal substructure).
std::set<StateExtractUsage>
enumerate_extract_combinations(unsigned num_bytes,
        std::map<unsigned, std::set<StateExtractUsage>>& all_usages) {
    std::set<StateExtractUsage> usages;

    if (num_bytes == 0) {
        StateExtractUsage use;
        usages.insert(use);
        return usages;
    }

    if (all_usages.count(num_bytes))
        return all_usages.at(num_bytes);

    for (auto sz : { PHV::Size::b8, PHV::Size::b16, PHV::Size::b32 }) {
        unsigned bytes = (unsigned)sz/8;

        if (num_bytes >= bytes) {
            auto opt_sub = enumerate_extract_combinations(num_bytes - bytes, all_usages);

            for (auto& u : opt_sub) {
                auto use = u;
                use.use[sz]++;
                usages.insert(use);
            }
        }
    }

    for (auto use : usages)
        verify(use, num_bytes);

    all_usages[num_bytes] = usages;  // memoize

    LOG4(print_scoreboard(num_bytes, usages));

    return usages;
}

std::set<StateExtractUsage>
enumerate_extract_combinations(unsigned num_bytes) {
    static std::map<unsigned, std::set<StateExtractUsage>> all_usages;

    if (all_usages.count(num_bytes))
        return all_usages.at(num_bytes);

    auto res = enumerate_extract_combinations(num_bytes, all_usages);
    return res;
}

int get_score(const StateExtractUsage& use) {
    unsigned num_bytes = use.total_bytes();
    auto combos = enumerate_extract_combinations(num_bytes);

    auto it = combos.find(use);

    BUG_CHECK(it != combos.end(), "invalid extractor use?");

    unsigned dis = std::distance(combos.begin(), it);

    // We use negative number for the score, with 0 being the best score,
    // and the more negative the worse. Essentially, the score indicates
    // the penalty incurred because of bandwidth imbalance.
    return -dis;
}

}  // namespace ParserExtractScore

#endif /* BF_P4C_PHV_PARSER_EXTRACT_BALANCE_SCORE_H_ */
