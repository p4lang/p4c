/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_SIMPLE_SWITCH_MIDEND_H_
#define BACKENDS_BMV2_SIMPLE_SWITCH_MIDEND_H_

#include "backends/bmv2/common/midend.h"
#include "backends/bmv2/common/options.h"
#include "frontends/common/options.h"
#include "ir/ir.h"
#include "midend/convertEnums.h"

namespace P4::BMV2 {

class SimpleSwitchMidEnd : public MidEnd {
 public:
    /// If p4c is run with option '--listMidendPasses', outStream is used for printing passes names.
    explicit SimpleSwitchMidEnd(CompilerOptions &options, std::ostream *outStream = nullptr);
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_SIMPLE_SWITCH_MIDEND_H_ */
