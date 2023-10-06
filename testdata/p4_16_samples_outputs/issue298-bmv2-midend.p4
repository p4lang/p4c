#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}

header myhdr_t {
    bit<16> msgtype;
    bit<32> inst;
    bit<16> rnd;
}

struct headers {
    @name("ethernet")
    ethernet_t ethernet;
    @name("ipv4")
    ipv4_t     ipv4;
    @name("udp")
    udp_t      udp;
    @name("myhdr")
    myhdr_t    myhdr;
}

struct ingress_metadata_t {
    bit<16> round;
    bit<1>  set_drop;
}

struct metadata {
    bit<16> _local_metadata_round0;
    bit<1>  _local_metadata_set_drop1;
}

parser TopParser(packet_in b, out headers p, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        b.extract<ethernet_t>(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: noMatch;
        }
    }
    state parse_ipv4 {
        b.extract<ipv4_t>(p.ipv4);
        transition select(p.ipv4.protocol) {
            8w0x11: parse_udp;
            default: accept;
        }
    }
    state parse_udp {
        b.extract<udp_t>(p.udp);
        transition select(p.udp.dstPort) {
            16w0x8888: parse_myhdr;
            default: accept;
        }
    }
    state parse_myhdr {
        b.extract<myhdr_t>(p.myhdr);
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control TopDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<udp_t>(hdr.udp);
        packet.emit<myhdr_t>(hdr.myhdr);
    }
}

struct tuple_0 {
    bit<4>  f0;
    bit<4>  f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
    bit<3>  f5;
    bit<13> f6;
    bit<8>  f7;
    bit<8>  f8;
    bit<32> f9;
    bit<32> f10;
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid(), (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid(), (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("egress._drop") action _drop() {
        mark_to_drop(standard_metadata);
    }
    @name("egress.drop_tbl") table drop_tbl_0 {
        key = {
            meta._local_metadata_set_drop1: exact @name("meta.ingress_metadata.set_drop");
        }
        actions = {
            _drop();
            NoAction_1();
        }
        size = 2;
        default_action = NoAction_1();
    }
    apply {
        drop_tbl_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ingress.registerRound") register<bit<16>>(32w65536) registerRound_0;
    @name("ingress.read_round") action read_round() {
        registerRound_0.read(meta._local_metadata_round0, hdr.myhdr.inst);
    }
    @name("ingress.round_tbl") table round_tbl_0 {
        actions = {
            read_round();
        }
        size = 8;
        default_action = read_round();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            if (hdr.myhdr.isValid()) {
                round_tbl_0.apply();
            }
        }
    }
}

V1Switch<headers, metadata>(TopParser(), verifyChecksum(), ingress(), egress(), computeChecksum(), TopDeparser()) main;
