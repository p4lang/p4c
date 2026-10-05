#include <core.p4>


struct header_t {
}

control MyC(inout header_t h) {
    apply {
    }
}

package Pipeline<Ctrl>(Ctrl c);
Pipeline(MyC()) pipe_inferred;
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
package SwitchPipe<Pipe>(Pipe p0, @optional Pipe p1);
SwitchPipe(pipe_inferred) switch_inferred;
SwitchPipe<Pipeline<MyC>>(pipe_explicit) switch_explicit;
package Top(Switch s, SwitchPipe<Pipeline<MyC>> a, SwitchPipe<Pipeline<MyC>> b);
Top(sw, switch_inferred, switch_explicit) main;
