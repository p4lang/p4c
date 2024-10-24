/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
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
