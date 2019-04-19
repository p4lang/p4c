#include <core.p4>

parser P(packet_in p, out bit<32> h) {
    state start {
        p.extract(h);
        transition accept;
    }
}

parser Simple(packet_in p, out bit<32> h);
package top(Simple prs);
top(P()) main;

