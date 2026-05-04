/*
 * Copyright 2024 Marvell Technology, Inc.
 * SPDX-FileCopyrightText: 2024 Marvell Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_PORTABLE_COMMON_OPTIONS_H_
#define BACKENDS_BMV2_PORTABLE_COMMON_OPTIONS_H_

#include "backends/bmv2/common/options.h"
#include "backends/bmv2/portable_common/midend.h"

namespace P4::BMV2 {

class PortableOptions : public BMV2Options {
 public:
    /// Process the command line arguments and set options accordingly.
    std::vector<const char *> *process(int argc, char *const argv[]) override;
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_PORTABLE_COMMON_OPTIONS_H_ */
