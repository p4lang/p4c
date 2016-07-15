#include "/home/mbudiu/git/p4c/p4include/core.p4"

parser P(packet_in p) {
    state start {
        transition start;
    }
}

parser Simple(packet_in p);
package top(Simple prs);
top(P()) main;
