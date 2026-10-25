/*
 * Copyright 2024 Marvell Technology, Inc.
 * SPDX-FileCopyrightText: 2024 Marvell Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_PNA_NIC_OPTIONS_H_
#define BACKENDS_BMV2_PNA_NIC_OPTIONS_H_

#include "backends/bmv2/pna_nic/midend.h"
#include "backends/bmv2/portable_common/options.h"

namespace P4::BMV2 {

class PnaNicOptions : public PortableOptions {
 public:
    PnaNicOptions() {
        registerOption(
            "--listMidendPasses", nullptr,
            [this](const char *) {
                listMidendPasses = true;
                loadIRFromJson = false;
                PnaNicMidEnd midEnd(*this, outStream);
                exit(0);
                return false;
            },
            "[PnaNic back-end] Lists exact name of all midend passes.\n");
    }
};

using PnaNicContext = P4CContextWithOptions<PnaNicOptions>;

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_PNA_NIC_OPTIONS_H_ */
