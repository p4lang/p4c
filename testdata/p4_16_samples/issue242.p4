#include <core.p4>
#include <v1model.p4>

// This program processes packets composed of an Ethernet and
// an IPv4 header, performing forwarding based on the
// destination IP address

typedef bit<48>  EthernetAddress;
typedef bit<32>  IPv4Address;

// standard Ethernet header
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

// IPv4 header without options
header ipv4_t {
    bit<4>       version;
    bit<4>       ihl;
    bit<8>       diffserv;
    bit<16>      packet_length;
    bit<16>      identification;
    bit<3>       flags;
    bit<13>      fragOffset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdrChecksum;
    IPv4Address  srcAddr;
    IPv4Address  dstAddr;
}

// Parser section

// List of all recognized headers
struct Headers {
    ethernet_t ethernet;
    ipv4_t     ip;
}

struct Value {
	bit<32> field1;
}


struct Metadata {}

parser P(packet_in b,
         out Headers p,
         inout Metadata meta,
         inout standard_metadata_t standard_meta) {
    state start {
        b.extract(p.ethernet);
        transition select(p.ethernet.etherType) {
            0x0800 : parse_ipv4;
            // no default rule: all other packets rejected
        }
    }

    state parse_ipv4 {
        b.extract(p.ip);
        transition accept;
    }
}

// match-action pipeline section

control Ing(inout Headers headers,
            inout Metadata meta,
            inout standard_metadata_t standard_meta) {
    apply {
        standard_meta.egress_spec = 0;
    }
}

control Eg(inout Headers hdrs,
           inout Metadata meta,
           inout standard_metadata_t standard_meta) {

    register<bit<32>>(32w100) debug;
    register<bit<32>>(32w1) reg;

    // Using register regKeys, regValues.
    action test() {
        Value val = {0};

        bool _pred = (val.field1 != 0);
        bit<32> inc = _pred ? 32w1 : 0;
        debug.write(0, _pred ? 32w1 : 32w0); // Print _pred
        debug.write(1, inc); // Print inc
	val.field1 = 32w1;
        debug.write(2, inc); // Print inc again

        reg.write(32w0, val.field1);
    }

    apply {
       test();
    }
}

// deparser section
control DP(packet_out b, in Headers p) {
    apply {
        b.emit(p.ethernet);
        b.emit(p.ip);
    }
}

// Fillers
control Verify(inout Headers hdrs, inout Metadata meta) {
    apply {}
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {}
}

// Instantiate the top-level V1 Model package.
V1Switch(P(),
         Verify(),
         Ing(),
         Eg(),
         Compute(),
         DP()) main;
