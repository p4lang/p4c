/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control C<H>() {
  apply {}
}

control cproto();
package top(cproto _c);

top(C<_>()) c;
