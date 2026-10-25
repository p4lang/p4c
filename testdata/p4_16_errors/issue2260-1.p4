/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control C();
package S(C c);
T f<T>(T x) {
    return x;
}
control MyC() {
    apply {
        bit<8> y = f(255);
   }
}
S(MyC()) main;
