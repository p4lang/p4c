#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct Headers {
}

struct Key {
    bit<32> field1;
}

struct Value {
    bit<32> field1;
}

struct Metadata {
}

parser P(packet_in b, out Headers p, inout Metadata meta, inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

control Ing(inout Headers headers, inout Metadata meta, inout standard_metadata_t standard_meta) {
    apply {
    }
}

control Eg(inout Headers hdrs, inout Metadata meta, inout standard_metadata_t standard_meta) {
    @name("Eg.inKey") Key inKey_0;
    @name("Eg.defaultKey") Key defaultKey_0;
    @name("Eg.same") bool same_0;
    @name("Eg.done") bool done_0;
    @name("Eg.ok") bool ok_0;
    @name("Eg.test") action test() {
        inKey_0 = (Key){field1 = 32w1};
        defaultKey_0 = (Key){field1 = 32w0};
        same_0 = inKey_0 == defaultKey_0;
        done_0 = false;
        ok_0 = !done_0 && same_0;
    }
    apply {
        test();
    }
}

control DP(packet_out b, in Headers p) {
    apply {
    }
}

control Verify(inout Headers hdrs, inout Metadata meta) {
    apply {
    }
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Headers, Metadata>(P(), Verify(), Ing(), Eg(), Compute(), DP()) main;
