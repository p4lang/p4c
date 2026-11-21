/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
header h {
    bit<32> field;
}

control c() {
    h[10] stack;

    apply {
        stack.last = { 10 };
    }
}