/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern counter {}

parser p() {
  bit<16> counter;
  state start {
    counter = 0;
    transition accept;
  }
}
