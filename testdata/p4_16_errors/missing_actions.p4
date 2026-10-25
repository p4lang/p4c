/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core.p4"

control c(in bit x) {
    table t {
        key = { x : exact; }
    }

    apply {}
}
