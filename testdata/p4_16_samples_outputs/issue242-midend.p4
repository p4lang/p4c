#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     packet_length;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

struct Headers {
    ethernet_t ethernet;
    ipv4_t     ip;
}

struct Value {
    bit<32> field1;
}

struct Metadata {
}

parser P(packet_in b, out Headers p, inout Metadata meta, inout standard_metadata_t standard_meta) {
    state start {
        b.extract<ethernet_t>(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: noMatch;
        }
    }
    state parse_ipv4 {
        b.extract<ipv4_t>(p.ip);
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control Ing(inout Headers headers, inout Metadata meta, inout standard_metadata_t standard_meta) {
    action act() {
        standard_meta.drop = 1w0;
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control Eg(inout Headers hdrs, inout Metadata meta, inout standard_metadata_t standard_meta) {
    Value val;
    bool _pred;
    bit<32> inc;
    bool tmp_2;
    bit<32> tmp_3;
    bit<32> tmp_4;
    @name("debug") register<bit<32>>(32w100) debug;
    @name("reg") register<bit<32>>(32w1) reg;
    @name("test") action test_0() {
        val.field1 = 32w0;
        tmp_2 = val.field1 != 32w0;
        _pred = val.field1 != 32w0;
        tmp_3 = (val.field1 != 32w0 ? 32w1 : tmp_3);
        tmp_3 = (!(val.field1 != 32w0) ? 32w0 : tmp_3);
        inc = tmp_3;
        tmp_4 = (val.field1 != 32w0 ? 32w1 : tmp_4);
        tmp_4 = (!(val.field1 != 32w0) ? 32w0 : tmp_4);
        debug.write(32w0, tmp_4);
        debug.write(32w1, tmp_3);
        val.field1 = 32w1;
        debug.write(32w2, tmp_3);
        reg.write(32w0, val.field1);
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
        b.emit<ethernet_t>(p.ethernet);
        b.emit<ipv4_t>(p.ip);
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
