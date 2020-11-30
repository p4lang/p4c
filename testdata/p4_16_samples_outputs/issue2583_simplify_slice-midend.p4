#include <core.p4>

header Header {
    bit<32> data;
}

parser p(packet_in pckt, out Header h) {
    state start {
        h.data = 32w0;
        h.data = 32w7;
        h.data[15:0] = 16w8;
        h.data[31:16] = 16w5;
        transition accept;
    }
}

parser proto(packet_in pckt, out Header h);
package top(proto _p);
top(p()) main;

