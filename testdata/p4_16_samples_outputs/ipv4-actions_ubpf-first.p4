#include <core.p4>
#include <ubpf_model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header IPv4_h {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     totalLen;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800: ipv4;
            default: reject;
        }
    }
    state ipv4 {
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    action Reject() {
        mark_to_drop();
    }
    action set_ipv4_version(bit<4> version) {
        headers.ipv4.version = version;
    }
    action set_ihl(bit<4> ihl) {
        headers.ipv4.ihl = ihl;
    }
    action set_diffserv(bit<8> diffserv) {
        headers.ipv4.diffserv = diffserv;
    }
    action set_identification(bit<16> identification) {
        headers.ipv4.identification = identification;
    }
    action set_flags(bit<3> flags) {
        headers.ipv4.flags = flags;
    }
    action set_fragOffset(bit<13> fragOffset) {
        headers.ipv4.fragOffset = fragOffset;
    }
    action set_ttl(bit<8> ttl) {
        headers.ipv4.ttl = ttl;
    }
    action set_protocol(bit<8> protocol) {
        headers.ipv4.protocol = protocol;
    }
    action set_srcAddr(bit<32> srcAddr) {
        headers.ipv4.srcAddr = srcAddr;
    }
    action set_dstAddr(bit<32> dstAddr) {
        headers.ipv4.dstAddr = dstAddr;
    }
    action set_srcAddr_dstAddr(bit<32> srcAddr, bit<32> dstAddr) {
        headers.ipv4.srcAddr = srcAddr;
        headers.ipv4.dstAddr = dstAddr;
    }
    action set_ihl_diffserv(bit<4> ihl, bit<8> diffserv) {
        headers.ipv4.ihl = ihl;
        headers.ipv4.diffserv = diffserv;
    }
    action set_fragOffset_flags(bit<13> fragOffset, bit<3> flags) {
        headers.ipv4.flags = flags;
        headers.ipv4.fragOffset = fragOffset;
    }
    action set_flags_ttl(bit<3> flags, bit<8> ttl) {
        headers.ipv4.flags = flags;
        headers.ipv4.ttl = ttl;
    }
    action set_fragOffset_srcAddr(bit<13> fragOffset, bit<32> srcAddr) {
        headers.ipv4.fragOffset = fragOffset;
        headers.ipv4.srcAddr = srcAddr;
    }
    table filter_tbl {
        key = {
            headers.ipv4.srcAddr: exact @name("headers.ipv4.srcAddr") ;
        }
        actions = {
            set_ipv4_version();
            set_ihl();
            set_diffserv();
            set_identification();
            set_flags();
            set_fragOffset();
            set_ttl();
            set_protocol();
            set_srcAddr();
            set_dstAddr();
            set_srcAddr_dstAddr();
            set_ihl_diffserv();
            set_fragOffset_flags();
            set_flags_ttl();
            set_fragOffset_srcAddr();
            Reject();
            NoAction();
        }
        const default_action = NoAction();
    }
    apply {
        filter_tbl.apply();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<IPv4_h>(headers.ipv4);
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;

