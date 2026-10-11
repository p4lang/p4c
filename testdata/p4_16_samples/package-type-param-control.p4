/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Packages may take entire control/parser types as type parameters.
 */

#include <core.p4>

struct header_t {}

control MyC(inout header_t h) {
    apply {}
}

package Pipeline<Ctrl>(Ctrl c);

// Type argument inferred from the constructor argument.
Pipeline(MyC()) pipe_inferred;

// Explicit type argument: control implementation name denotes its Type_Control.
Pipeline<MyC>(MyC()) pipe_explicit;

parser MyP(packet_in pkt, out header_t h) {
    state start {
        transition accept;
    }
}

package ParserPipe<Prsr>(Prsr p);

ParserPipe(MyP()) parser_inferred;
ParserPipe<MyP>(MyP()) parser_explicit;

package Switch(Pipeline<MyC> p0, Pipeline<MyC> p1, ParserPipe<MyP> p2, ParserPipe<MyP> p3);
Switch(pipe_inferred, pipe_explicit, parser_inferred, parser_explicit) sw;

// Higher-order package: type parameter ranges over a package type (cf. issue2630).
package SwitchPipe<Pipe>(Pipe p0, @optional Pipe p1);
SwitchPipe(pipe_inferred) switch_inferred;
SwitchPipe<Pipeline<MyC>>(pipe_explicit) switch_explicit;

package Top(Switch s, SwitchPipe<Pipeline<MyC>> a, SwitchPipe<Pipeline<MyC>> b);
Top(sw, switch_inferred, switch_explicit) main;
