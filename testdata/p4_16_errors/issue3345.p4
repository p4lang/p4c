/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser MyParser2(in bool t) {
  state start {
    transition select(t) {
     1w1: accept;
     _: reject;
   }
 }
}
