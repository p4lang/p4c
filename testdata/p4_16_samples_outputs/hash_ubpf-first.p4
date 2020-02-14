#include <core.p4>
#include <ubpf_model.p4>

header test_t {
    bit<16> sa;
    bit<8>  da;
}

header test1_t {
    bit<8>  a;
    bit<16> b;
    bit<16> c;
    bit<8>  d;
}

header test2_t {
    bit<8> a;
}

struct Headers_t {
    test_t  test;
    test1_t test1;
    test2_t test2;
}

struct metadata {
    bit<32> output;
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta) {
    state start {
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta) {
    action a() {
        hash<tuple<bit<16>, bit<8>>>(meta.output, HashAlgorithm.lookup3, { headers.test.sa, headers.test.da });
    }
    action b() {
        hash<tuple<bit<8>, bit<16>, bit<16>, bit<8>>>(meta.output, HashAlgorithm.lookup3, { headers.test1.a, headers.test1.b, headers.test1.c, headers.test1.d });
    }
    action c() {
        hash<tuple<bit<8>>>(meta.output, HashAlgorithm.lookup3, { headers.test2.a });
    }
    apply {
        a();
        b();
        c();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;

