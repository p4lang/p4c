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
    Key inKey;
    Key defaultKey;
    bool same;
    Value val;
    bool done;
    bool ok;
    bool tmp_1;
    bool tmp_2;
    Value val_2;
    @name("test") action test_0() {
        inKey.field1 = 32w1;
        defaultKey.field1 = 32w0;
        tmp_1 = inKey == defaultKey;
        same = inKey == defaultKey;
        val.field1 = 32w0;
        done = false;
        tmp_2 = (!!false ? false : tmp_2);
        tmp_2 = (!!!false ? inKey == defaultKey : (!!false ? false : tmp_2));
        ok = (!!!false ? inKey == defaultKey : (!!false ? false : tmp_2));
        val_2.field1 = ((!!!false ? inKey == defaultKey : (!!false ? false : tmp_2)) ? val.field1 : val_2.field1);
        val_2.field1 = ((!!!false ? inKey == defaultKey : (!!false ? false : tmp_2)) ? 32w8 : val_2.field1);
        val.field1 = ((!!!false ? inKey == defaultKey : (!!false ? false : tmp_2)) ? val_2.field1 : val.field1);
    }
    table tbl_test() {
        actions = {
            test_0();
        }
        const default_action = test_0();
    }
    apply {
        tbl_test.apply();
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
