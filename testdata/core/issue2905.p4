/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

control c() {
    action a() {}
    table t {
        key = {}
        actions = {
            a;
        }
        const entries = {}
    }

    apply {
    }
}
