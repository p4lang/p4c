/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_COMMON_CHECK_UNSUPPORTED_H_
#define BACKENDS_BMV2_COMMON_CHECK_UNSUPPORTED_H_

#include "frontends/common/options.h"
#include "ir/ir.h"
#include "lower.h"
#include "midend/convertEnums.h"

namespace P4::BMV2 {

class CheckUnsupported : public Inspector {
    bool preorder(const IR::ForInStatement *fs) override {
        error(ErrorType::ERR_UNSUPPORTED, "%sBMV2 does not support for-in loops", fs->srcInfo);
        return false;
    }
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_COMMON_CHECK_UNSUPPORTED_H_ */
