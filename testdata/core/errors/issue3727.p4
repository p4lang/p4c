/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void f2(in bit a) {}
action NoAction(){}

control c(in bit a) {
  bit tt;
  table t {
    actions = {f2(1);}
  }
  apply { tt = 1; t.apply(); }
}

control ct(in bit a);
package pt(ct c);
pt(c()) main;