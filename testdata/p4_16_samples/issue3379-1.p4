/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser mypt<t>(in t tt);

parser MyParser1(in bit tt) {
  state start {
    transition select(tt) {
     0: accept;
     _: reject;
   }
 }
}

package mypackaget<t>(mypt<t> t2);
package mypackaget<t>(mypt<t> t1, mypt<t> t2);
mypackaget<bit>(MyParser1()) t4;
