/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_PSA_SWITCH_OPTIONS_H_
#define BACKENDS_BMV2_PSA_SWITCH_OPTIONS_H_

#include "backends/bmv2/portable_common/options.h"
#include "backends/bmv2/psa_switch/midend.h"

namespace P4::BMV2 {

class PsaSwitchOptions : public PortableOptions {
 public:
    PsaSwitchOptions() {
        registerOption(
            "--listMidendPasses", nullptr,
            [this](const char *) {
                listMidendPasses = true;
                loadIRFromJson = false;
                PsaSwitchMidEnd midEnd(*this, outStream);
                exit(0);
                return false;
            },
            "[PsaSwitch back-end] Lists exact name of all midend passes.\n");
    }
};

using PsaSwitchContext = P4CContextWithOptions<PsaSwitchOptions>;

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_PSA_SWITCH_OPTIONS_H_ */
