#include <core.p4>
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
        }
    }
    state parse_ipv4 {
        b.extract<ipv4_t>(p.ip);
        transition accept;
    }
}

control Ing(inout Headers headers, inout Metadata meta, inout standard_metadata_t standard_meta) {
    apply {
        standard_meta.egress_spec = 9w1;
    }
}

control Eg(inout Headers hdrs, inout Metadata meta, inout standard_metadata_t standard_meta) {
    action test() {
        Value val = { 32w0 };
        bool _pred = val.field1 != 32w0;
        bit<32> inc = (_pred ? 32w1 : 32w0);
        debug.write(32w0, (_pred ? 32w1 : 32w0));
        debug.write(32w1, inc);
        val.field1 = 32w1;
        debug.write(32w2, inc);
        reg.write(32w0, val.field1);
    }
    apply {
        test();
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

