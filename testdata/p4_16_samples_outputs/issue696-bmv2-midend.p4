#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
register<bit<32>>(32w100) debug;

register<bit<32>>(32w1) reg;

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
    @hidden action issue696bmv2l76() {
        standard_meta.egress_spec = 9w1;
    }
    @hidden table tbl_issue696bmv2l76 {
        actions = {
            issue696bmv2l76();
        }
        const default_action = issue696bmv2l76();
    }
    apply {
        tbl_issue696bmv2l76.apply();
    }
}

control Eg(inout Headers hdrs, inout Metadata meta, inout standard_metadata_t standard_meta) {
    @name("Eg.test") action test() {
        debug.write(32w0, 32w0);
        debug.write(32w1, 32w0);
        debug.write(32w2, 32w0);
        reg.write(32w0, 32w1);
    }
    @hidden table tbl_test {
        actions = {
            test();
        }
        const default_action = test();
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

control Verify(inout Headers hdrs, inout Metadata meta) {
    apply {
    }
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Headers, Metadata>(P(), Verify(), Ing(), Eg(), Compute(), DP()) main;

