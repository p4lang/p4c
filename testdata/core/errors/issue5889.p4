/*
 * SPDX-FileCopyrightText: 2026 Pantherun Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */
struct H { }

control C() {
    apply {
        H h = { f0 = { f1 = { ... }, ... }, ... };
        h = h;
    }
}

control CT();
package Top(CT c);
Top(C()) main;
