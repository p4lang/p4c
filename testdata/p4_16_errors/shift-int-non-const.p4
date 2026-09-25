/*
 * SPDX-FileCopyrightText: 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header hdr_t {
  bit<8> v;
}

control c(inout hdr_t hdr, inout bit<4> b) {
  apply {
    const int a = 5;
    hdr.v = (bit<8>)(a >> b);
  }
}

control C(inout hdr_t hdr, inout bit<4> b);
package P(C c);

P(c()) main;
