/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header H { bit<32> x; }

control c() {
    tuple<bit<32>> t = { 0 };
    H h;
    apply {
        h = t; // illegal assignment between tuple and header
    }
}