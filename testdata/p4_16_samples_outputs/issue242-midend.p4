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
    @hidden action act() {
        standard_meta.egress_spec = 9w0;
    }
    @hidden table tbl_act {
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
    bit<32> tmp;
    bit<32> tmp_0;
    @name("Eg.debug") register<bit<32>>(32w100) debug_0;
    @name("Eg.reg") register<bit<32>>(32w1) reg_0;
    @name("Eg.test") action test() {
        tmp = tmp;
        tmp = 32w0;
        tmp_0 = tmp_0;
        tmp_0 = 32w0;
        debug_0.write(32w0, tmp_0);
        debug_0.write(32w1, tmp);
        debug_0.write(32w2, tmp);
        reg_0.write(32w0, 32w1);
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

