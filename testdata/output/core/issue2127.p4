#include <core.p4>

header H {
    bit<32> b;
}

parser p(packet_in packet, out H h) {
    state start {
        packet.extract(h);
        ;
        transition accept;
    }
}

parser proto<T>(packet_in p, out T t);
package top<T>(proto<T> p);
top(p()) main;
