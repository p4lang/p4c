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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_COMMON_MATCH_REDUCER_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_COMMON_MATCH_REDUCER_H_

#include <stdint.h>

#include "lib/match.h"
#include "lib/ordered_set.h"

/**
 * @ingroup parde
 */
class HasFullMatchCoverage {
    const std::vector<match_t> matches_;

    /**
     * @brief match_t does not have std::less defined. Therefore, a custom comparator is required to
     * be able to use that type in a std::set/map.
     *
     */
    struct cmp {
        bool operator()(const match_t &a, const match_t &b) const {
            return std::tie(a.word1, a.word0) < std::tie(b.word1, b.word0);
        }
    };

    bool is_power_of_2(big_int value) { return value && !(value & (value - 1)); }

    /**
     * @brief Given a set of @p matches, combine all pairs of matches that only differ by a single
     * bit in the same position together into a single match where that bit is replaced by a "don't
     * care" instead. Repeat recursively until no more pairs can be found and the set can no longer
     * be reduced in size.
     *
     * @param matches A set of matches to reduce
     * @return std::set<match_t, cmp> A reduced set of matches
     */
    std::set<match_t, cmp> reduce(const std::set<match_t, cmp> matches) {
        std::set<match_t, cmp> transformed_matches;
        for (const auto &match1 : matches) {
            bool combined_matches = false;
            for (const auto &match2 : matches) {
                big_int xor0 = match1.word0 ^ match2.word0, xor1 = match1.word1 ^ match2.word1;

                if (xor0 == 0 && xor1 == 0) continue;
                if (xor0 != xor1) continue;
                if (!is_power_of_2(xor0)) continue;

                match_t combined_match((match1.word0 | xor0), (match1.word1 | xor1));
                transformed_matches.insert(combined_match);
                combined_matches = true;
            }
            if (!combined_matches) transformed_matches.insert(match1);
        }
        if (transformed_matches != matches) return reduce(transformed_matches);
        return matches;
    }

    /**
     * @brief Find if the std::vector matches passed to the class' constructor contains an
     * unconditional match or if its values cover the full match space.
     *
     */
    void has_full_match_coverage() {
        auto is_unconditional_match = [](match_t match) {
            return match == match_t::dont_care(0) || match == match_t::dont_care(8) ||
                   match == match_t::dont_care(16) || match == match_t::dont_care(24) ||
                   match == match_t::dont_care(32);
        };

        rv = std::any_of(matches_.begin(), matches_.end(), is_unconditional_match);
        if (rv) return;

        std::set<match_t, cmp> reduced_matches =
            reduce(std::set<match_t, cmp>(matches_.begin(), matches_.end()));
        rv = std::any_of(reduced_matches.begin(), reduced_matches.end(), is_unconditional_match);
    }

 public:
    bool rv = false;

    explicit HasFullMatchCoverage(const std::vector<match_t> &matches) : matches_(matches) {
        has_full_match_coverage();
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_COMMON_MATCH_REDUCER_H_ */
