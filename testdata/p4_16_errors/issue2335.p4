/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control C() {
  apply {
    int x = 0;
    int<8> y = x + 8s1;
  }
}
