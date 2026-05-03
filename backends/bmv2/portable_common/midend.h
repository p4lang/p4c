/*
 * Copyright 2024 Marvell Technology, Inc.
 * SPDX-FileCopyrightText: 2024 Marvell Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_PORTABLE_COMMON_MIDEND_H_
#define BACKENDS_BMV2_PORTABLE_COMMON_MIDEND_H_

#include "backends/bmv2/common/midend.h"
#include "backends/bmv2/common/options.h"
#include "frontends/common/options.h"
#include "ir/ir.h"
#include "midend/convertEnums.h"

namespace P4::BMV2 {

class PortableMidEnd : public MidEnd {
 public:
    explicit PortableMidEnd(CompilerOptions &options) : MidEnd(options) {}
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_PORTABLE_COMMON_MIDEND_H_ */
