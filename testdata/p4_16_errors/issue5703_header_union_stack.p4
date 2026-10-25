/*
 * SPDX-FileCopyrightText: 2026 Abhishek Agarwal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
@command_line("--loopsUnroll")

header h1 { bit<8>  a; }
header h2 { bit<16> b; }
header_union HU { h1 x; h2 y; }

struct headers_t { HU[4] hus; }
struct meta_t    { bit<8> idx; }

parser P(packet_in pkt, out headers_t hdrs, inout meta_t m);
package top(P p);

parser myp(packet_in pkt, out headers_t hdrs, inout meta_t m) {
    state start {
        // Non-constant index into a header union stack. This used to crash the
        // compiler (issue 5703); now it reports an error instead.
        pkt.extract(hdrs.hus[m.idx].x);
        transition accept;
    }
}

top(myp()) main;
