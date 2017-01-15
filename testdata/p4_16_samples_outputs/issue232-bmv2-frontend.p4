#include <core.p4>
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
    Key inKey_0;
    Key defaultKey_0;
    bool same_0;
    Value val_0;
    bool done_0;
    bool ok_0;
    bool tmp;
    bool tmp_0;
    @name("update") action update_0(inout Value val) {
        val.field1 = 32w8;
    }
    @name("test") action test_0() {
        inKey_0 = { 32w1 };
        defaultKey_0 = { 32w0 };
        tmp = inKey_0 == defaultKey_0;
        same_0 = tmp;
        val_0 = { 32w0 };
        done_0 = false;
        if (!!done_0) 
            tmp_0 = false;
        else 
            tmp_0 = same_0;
        ok_0 = tmp_0;
        if (ok_0) 
            update_0(val_0);
    }
    apply {
        test_0();
    }
}

control DP(packet_out b, in Headers p) {
    apply {
    }
}

control Verify(in Headers hdrs, inout Metadata meta) {
    apply {
    }
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Headers, Metadata>(P(), Verify(), Ing(), Eg(), Compute(), DP()) main;
