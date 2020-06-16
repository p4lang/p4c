#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
typedef bit<16> ethertype_t;
typedef bit<9> port_t;

#define MAX_SGMTS  3
#define UDP_PROTOCOL 8w0x11
#define TCP_PROTOCOL 8w0x6

const bit<16> IPV4_ETHER = 0x0800;
const bit<16> SGMT_ETHER = 0x0700;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    ethertype_t ethertype;
}

header myTunner_t {
    port_t lport;
    ethertype_t  ethertype;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     total_len;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     frag_offset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdr_checksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}

struct headers_t {
    ethernet_t ethernet;
    myTunner_t[MAX_SGMTS] myTunners;
    ipv4_t     ipv4;
    tcp_t   tcp;
    udp_t   udp;
}

error {
    IPv4IncorrectVersion,
    IPv4OptionsNotSupported
}

struct metadata_t {
    bit<14> hash;
    bit<9> lport;
}

parser ingress_parser(packet_in packet,
        out headers_t hdr,
        inout metadata_t meta,
        inout standard_metadata_t standard_meta)
{
    state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.ethertype) {
            IPV4_ETHER:  parse_ipv4;
            SGMT_ETHER:  parse_sgmt;
            default: accept;
        }
    }

    state parse_sgmt {
        packet.extract(hdr.myTunners.next);
        transition select(hdr.myTunners.next.ethertype) {
            IPV4_ETHER:  parse_ipv4;
            SGMT_ETHER:  parse_sgmt;
            default:  accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        verify(hdr.ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hdr.ipv4.ihl == 4w5, error.IPv4OptionsNotSupported);
        transition select(hdr.ipv4.protocol) {
            UDP_PROTOCOL : parse_udp;
            TCP_PROTOCOL : parse_tcp;
            default : accept;
        }
    }

    state parse_tcp {
        packet.extract(hdr.tcp);
        transition accept;
    }

    state parse_udp {
        packet.extract(hdr.udp);
        transition accept;
    }
}

control deparser(packet_out packet,
        in headers_t hdr)
{
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.myTunners);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp);
        packet.emit(hdr.udp);
    }
}

control ingress(inout headers_t hdr,
        inout metadata_t meta,
        inout standard_metadata_t standard_metadata)
{
    bool dropped = false;
    bit<8> nw_id = 0;

    action myTunner_forward() {
        meta.lport = hdr.myTunners[0].lport;
        hdr.ethernet.ethertype =  hdr.myTunners[0].ethertype;
        hdr.myTunners.pop_front(1);
    }

    action drop_action() {
        mark_to_drop();
        dropped = true;
    }

    action forward_port(bit<9> port) {
        meta.lport = 0;
        standard_metadata.egress_spec = port;
    }

    action forward_lport(bit<9> lport) {
        meta.lport = lport;
    }

    action forward_path1(bit<9> lport1, bit<9> lport2) {
        meta.lport = lport1;
        hdr.myTunners[0].lport = lport2;
        hdr.myTunners[0].ethertype = hdr.ethernet.ethertype;
        hdr.ethernet.ethertype = SGMT_ETHER;

    }

    action forward_path2(bit<9> lport1, bit<9> lport2, bit<9> lport3) {
        meta.lport = lport1;
        hdr.myTunners[0].lport = lport2;
        hdr.myTunners[0].ethertype = SGMT_ETHER;
        hdr.myTunners[1].lport = lport3;
        hdr.myTunners[1].ethertype = hdr.ethernet.ethertype;
        hdr.ethernet.ethertype = SGMT_ETHER;
    }

    table dmac {
        key = {
                nw_id : exact;
                hdr.ethernet.dstAddr: exact;
        }
        actions = {
                drop_action;
                forward_port;
                forward_lport;
                forward_path1;
                forward_path2;
        }
        size = 1024;
        default_action = drop_action;
    }

    action set_vnet(bit<8> port_nw_id) {
        nw_id = port_nw_id;
    }

    table port_vnet {
        key = {
                standard_metadata.ingress_port: exact;
        }
        actions = {
                set_vnet;
                drop_action;
        }
        size = 10;
        default_action = drop_action;
    }


    action lport_hit(bit<32> port_count) {
        hash(meta.hash,
                HashAlgorithm.csum16,
                16w0,
                { hdr.ipv4.srcAddr,
                        hdr.ipv4.dstAddr,
                        hdr.ipv4.protocol,
                        hdr.tcp.srcPort,
                        hdr.tcp.dstPort },
                        port_count);
    }

    table lport_hash {
        key = {
                meta.lport: exact;
        }
        actions = {
                lport_hit;
        }
        size = 1024;
    }

    action set_output(bit<9> port) {
        standard_metadata.egress_spec = port;
    }

    table port_select {
        key = {
                meta.lport : exact;
                meta.hash: exact;
        }
        actions = {
                set_output;

        }
        size = 1024;
    }

    apply {
        if (hdr.myTunners[0].isValid()) {
            myTunner_forward();
        } else {
            port_vnet.apply();
            dmac.apply();
            if (dropped)
                return;
        }

        if (meta.lport != 0) {
            meta.hash = 0;
            lport_hash.apply();
            port_select.apply();
        }
    }
}

control verif_checksum(inout headers_t hdr,
        inout metadata_t meta)
{
    apply { }
}

control compute_checksum(inout headers_t hdr,
        inout metadata_t meta)
{
    apply { }
}

control egress(inout headers_t hdr,
        inout metadata_t meta,
        inout standard_metadata_t standard_metadata)
{
    apply { }
}

V1Switch(ingress_parser(),
        verif_checksum(),
        ingress(),
        egress(),
        compute_checksum(),
        deparser()) main;
