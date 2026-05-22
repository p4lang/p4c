/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

typedef bit t;
extern e {
  e();
  abstract t f(in t a);
}
parser MyParser(t tt) {
    e() ee = {
      t f(in t a) {
        return a;
      }
    };
    state start {
        transition accept;
    }
}
