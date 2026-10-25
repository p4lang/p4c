/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser P();
control C();
package V1Switch(P p, C c);

parser MyP() {
  state start {
    transition accept;
  }
}

control MyC() {
  apply {
  }
}

V1Switch(MyP(), MyC()) main;
