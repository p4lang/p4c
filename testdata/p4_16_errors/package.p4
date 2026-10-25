/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control proto(out bit<32> o);
package top(in proto _c);  // Package should not have in parameters

control c(out bit<32> o) {
    apply { o = 0; }
}

top(c(), true) main;
