/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control C();
package P(C a);

control MyC(@optional inout bool b) {
  apply {
     b = !b;
  }
}

control MyD() {
  MyC() c;
  apply {
    c.apply();
  }
}

P(MyD()) main;
