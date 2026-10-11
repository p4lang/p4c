/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * A control instance does not unify with a package type parameter bound to
 * a non-control type.
 */

struct header_t {}

control MyC(inout header_t h) {
    apply {}
}

package Pipeline<Ctrl>(Ctrl c);

Pipeline<bit<8>>(MyC()) main;
