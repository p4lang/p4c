/*
 * SPDX-FileCopyrightText: 2026 Abhishek Agarwal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header H {}
control C(out bit<32> x)() {
  H[{  }] stack;
  apply {
    stack = ...;
  }
}
