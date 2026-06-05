/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_SIMPLE_SWITCH_OPTIONS_H_
#define BACKENDS_BMV2_SIMPLE_SWITCH_OPTIONS_H_

#include "backends/bmv2/common/options.h"
#include "backends/bmv2/simple_switch/midend.h"

namespace P4::BMV2 {

class SimpleSwitchOptions : public BMV2Options {
 public:
    SimpleSwitchOptions() {
        registerOption(
            "--listMidendPasses", nullptr,
            [this](const char *) {
                listMidendPasses = true;
                loadIRFromJson = false;
                SimpleSwitchMidEnd midEnd(*this, outStream);
                exit(0);
                return false;
            },
            "[SimpleSwitch back-end] Lists exact name of all midend passes.\n");
    }
};

using SimpleSwitchContext = P4CContextWithOptions<SimpleSwitchOptions>;

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_SIMPLE_SWITCH_OPTIONS_H_ */
