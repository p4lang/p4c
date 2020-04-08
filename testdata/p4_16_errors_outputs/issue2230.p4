#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header h1_t {
    bit<8> f1;
    bit<8> f2;
}

header h2_t {
    bit<8> f1;
    bit<8> f2;
}

struct s1_t {
    bit<8> f1;
    bit<8> f2;
}

struct headers_t {
    ethernet_t ethernet;
    h1_t       h1;
    h1_t       h1b;
    h2_t       h2;
}

struct metadata_t {
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    s1_t s1;
    apply {
        hdr.h2 = hdr.h1;
        hdr.h1b = s1;
        s1 = hdr.h1;
    }
}

