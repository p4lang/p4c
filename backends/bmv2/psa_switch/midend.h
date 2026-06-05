/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_PSA_SWITCH_MIDEND_H_
#define BACKENDS_BMV2_PSA_SWITCH_MIDEND_H_

#include "backends/bmv2/portable_common/midend.h"

namespace P4::BMV2 {

class PsaSwitchMidEnd : public PortableMidEnd {
 public:
    // If p4c is run with option '--listMidendPasses', outStream is used for printing passes names
    explicit PsaSwitchMidEnd(CompilerOptions &options, std::ostream *outStream = nullptr);
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_PSA_SWITCH_MIDEND_H_ */
