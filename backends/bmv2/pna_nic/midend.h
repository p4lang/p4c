/*
 * Copyright 2024 Marvell Technology, Inc.
 * SPDX-FileCopyrightText: 2024 Marvell Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_PNA_NIC_MIDEND_H_
#define BACKENDS_BMV2_PNA_NIC_MIDEND_H_

#include "backends/bmv2/portable_common/midend.h"

namespace P4::BMV2 {

class PnaNicMidEnd : public PortableMidEnd {
 public:
    // If p4c is run with option '--listMidendPasses', outStream is used for printing passes names
    explicit PnaNicMidEnd(CompilerOptions &options, std::ostream *outStream = nullptr);
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_PNA_NIC_MIDEND_H_ */
