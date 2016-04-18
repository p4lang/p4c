#include "/home/cdodd/p4c/build/../p4include/core.p4"

parser Prs<T>(packet_in b, out T result);
control Map<T>(in T d);
package Switch<T>(Prs<T> prs, Map<T> map);
parser P(packet_in b, out bit<32> d) {
    state start {
    }
}

control Map1(in bit<32> d) {
    apply {
        bool hasExited = false;
    }
}

Switch(P(), Map1()) main;
