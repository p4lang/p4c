#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddressUint_t;
type EthernetAddressUint_t EthernetAddress_t;
typedef bit<32> IPv4AddressUint_t;
type IPv4AddressUint_t IPv4Address_t;
type IPv4Address_t IPv4Address2_t;
header ethernet_t {
    EthernetAddress_t dstAddr;
    EthernetAddress_t srcAddr;
    bit<16>           etherType;
}

header ipv4_t {
    bit<4>         version;
    bit<4>         ihl;
    bit<8>         diffserv;
    bit<16>        totalLen;
    bit<16>        identification;
    bit<3>         flags;
    bit<13>        fragOffset;
    bit<8>         ttl;
    bit<8>         protocol;
    bit<16>        hdrChecksum;
    IPv4Address2_t srcAddr;
    IPv4Address_t  dstAddr;
}

struct meta_t {
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

action my_drop(inout standard_metadata_t smeta) {
    mark_to_drop(smeta);
}
parser ParserImpl(packet_in packet, out headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    const bit<16> ETHERTYPE_IPV4 = 16w0x800;
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    action set_output(bit<9> out_port) {
        standard_metadata.egress_spec = out_port;
    }
    table ipv4_da_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            set_output();
            my_drop(standard_metadata);
        }
        default_action = my_drop(standard_metadata);
    }
    table ipv4_sa_filter {
        key = {
            hdr.ipv4.srcAddr: ternary @name("hdr.ipv4.srcAddr") ;
        }
        actions = {
            my_drop(standard_metadata);
            NoAction();
        }
        const default_action = NoAction();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_sa_filter.apply();
            ipv4_da_lpm.apply();
        }
    }
}

control egress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        verify_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, IPv4Address2_t, IPv4Address_t>, bit<16>>(hdr.ipv4.isValid() && hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, IPv4Address2_t, IPv4Address_t>, bit<16>>(hdr.ipv4.isValid() && hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers_t, meta_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

