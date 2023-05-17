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

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        transition accept;
    }
}

struct tuple_0 {
    bit<16> f0;
    bit<8>  f1;
}

struct tuple_1 {
    bit<8>  f0;
    bit<16> f1;
    bit<16> f2;
    bit<8>  f3;
}

struct tuple_2 {
    bit<8> f0;
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @name("pipe.a") action a_1() {
        hash<tuple_0>(meta.output, HashAlgorithm.lookup3, (tuple_0){f0 = headers.test.sa,f1 = headers.test.da});
    }
    @name("pipe.b") action b_1() {
        hash<tuple_1>(meta.output, HashAlgorithm.lookup3, (tuple_1){f0 = headers.test1.a,f1 = headers.test1.b,f2 = headers.test1.c,f3 = headers.test1.d});
    }
    @name("pipe.c") action c_1() {
        hash<tuple_2>(meta.output, HashAlgorithm.lookup3, (tuple_2){f0 = headers.test2.a});
    }
    @hidden table tbl_a {
        actions = {
            a_1();
        }
        const default_action = a_1();
    }
    @hidden table tbl_b {
        actions = {
            b_1();
        }
        const default_action = b_1();
    }
    @hidden table tbl_c {
        actions = {
            c_1();
        }
        const default_action = c_1();
    }
    apply {
        tbl_a.apply();
        tbl_b.apply();
        tbl_c.apply();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
