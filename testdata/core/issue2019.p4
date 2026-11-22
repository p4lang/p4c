/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control V<H>(inout H hdr);

control EmptyV<H>(inout H hdr) {
    apply { }
}

struct E {}

package S<H1>(V<H1> vr = EmptyV<H1>());
S<E>() main;
