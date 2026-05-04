/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>

control c() {
    table t {
        actions = {}  // empty actions list
    }
    apply {
        t.apply();
    }
}
