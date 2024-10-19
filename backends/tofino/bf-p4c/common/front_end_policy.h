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

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_FRONT_END_POLICY_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_FRONT_END_POLICY_H_

#include "frontends/p4/frontend.h"

namespace BFN {

struct FrontEndPolicy : P4::FrontEndPolicy {
    P4::ParseAnnotations *getParseAnnotations() const override { return pars; }

    bool skipSideEffectOrdering() const override { return skip_side_effect_ordering; }

    explicit FrontEndPolicy(P4::ParseAnnotations *parseAnnotations)
        : pars(parseAnnotations), skip_side_effect_ordering(false) {}

    explicit FrontEndPolicy(P4::ParseAnnotations *parseAnnotations, bool skip_side_effect_ordering)
        : pars(parseAnnotations), skip_side_effect_ordering(skip_side_effect_ordering) {}

 private:
    P4::ParseAnnotations *pars;
    bool skip_side_effect_ordering;
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_COMMON_FRONT_END_POLICY_H_ */
