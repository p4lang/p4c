/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control C<H>(inout H hdr);
control V<H>(inout H hdr);

struct E {}

control Some(inout E m) {
    apply {}
}

control EmptyV<H>(inout H hdr) {
    apply {}
}

package S<H1>(C<H1> s, V<H1> vr = EmptyV<H1>());
S<E>(Some()) main;
