/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser MyParser1(in bit<6> v) {
  value_set<int>(4) myvs;
  state start {
    transition select(v) {
     myvs: accept;
     _: reject;
   }
 }
}
