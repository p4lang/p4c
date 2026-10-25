/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

action nop() {}

control x()
{
    table t {
        actions = { nop; }
        default_action = nop;
    }

    apply {
        if (t.apply().hit) {}

        switch (t.apply().action_run) {
            nop: {}
            default: {}
        }
    }
}
