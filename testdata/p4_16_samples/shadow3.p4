/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header H {}

control MyIngress(inout H p) {
  bit<8> p = 0;
  apply {
  }
}
