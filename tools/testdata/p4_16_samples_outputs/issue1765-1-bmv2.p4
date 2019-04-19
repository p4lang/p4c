#include <core.p4>
#include <v1model.p4>

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<128> ipv6_addr_t;
typedef bit<9> port_t;
typedef bit<16> mcast_t;
typedef bit<16> task_t;
const bit<16> TYPE_IPV4 = 0x800;
const bit<16> TYPE_IPV6 = 0x86dd;
const bit<16> TYPE_CPU = 0x4242;
const bit<16> TYPE_DEBUG = 0x2323;
const bit<8> PROTO_ICMP = 1;
const bit<8> PROTO_TCP = 6;
const bit<8> PROTO_UDP = 17;
const bit<8> PROTO_ICMP6 = 58;
const bit<8> TCP_SEQ_LEN = 4;
const bit<8> ICMP6_ECHO_REQUEST = 128;
const bit<8> ICMP6_ECHO_REPLY = 129;
const bit<8> ICMP6_NS = 135;
const bit<8> ICMP6_NA = 136;
const task_t TASK_ICMP6_NS = 1;
const task_t TASK_ICMP6_GENERAL = 2;
const task_t TASK_DEBUG = 3;
const task_t TASK_ICMP6_REPLY = 4;
header ethernet_t {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16>    ethertype;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<6>      diff_serv;
    bit<2>      ecn;
    bit<16>     totalLen;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header ipv6_t {
    bit<4>      version;
    bit<8>      traffic_class;
    bit<20>     flow_label;
    bit<16>     payload_length;
    bit<8>      next_header;
    bit<8>      hop_limit;
    ipv6_addr_t src_addr;
    ipv6_addr_t dst_addr;
}

header tcp_t {
    bit<16> src_port;
    bit<16> dst_port;
    int<32> seqNo;
    int<32> ackNo;
    bit<4>  data_offset;
    bit<4>  res;
    bit<1>  cwr;
    bit<1>  ece;
    bit<1>  urg;
    bit<1>  ack;
    bit<1>  psh;
    bit<1>  rst;
    bit<1>  syn;
    bit<1>  fin;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header udp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> payload_length;
    bit<16> checksum;
}

header icmp6_t {
    bit<8>  type;
    bit<8>  code;
    bit<16> checksum;
}

header icmp_t {
    bit<8>  type;
    bit<8>  code;
    bit<16> checksum;
    bit<32> rest;
}

header cpu_t {
    task_t  task;
    bit<16> ingress_port;
    bit<16> ethertype;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    ipv6_t     ipv6;
    tcp_t      tcp;
    udp_t      udp;
    icmp6_t    icmp6;
    icmp_t     icmp;
    cpu_t      cpu;
}

struct metadata {
    port_t  ingress_port;
    task_t  task;
    bit<16> tcp_length;
    bit<32> cast_length;
    bit<1>  do_cksum;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.ethertype) {
            TYPE_IPV4: ipv4;
            TYPE_IPV6: ipv6;
            default: accept;
        }
    }
    state ipv4 {
        packet.extract(hdr.ipv4);
        meta.tcp_length = hdr.ipv4.totalLen - 16w20;
        transition select(hdr.ipv4.protocol) {
            PROTO_TCP: tcp;
            PROTO_UDP: udp;
            PROTO_ICMP: icmp;
            default: accept;
        }
    }
    state ipv6 {
        packet.extract(hdr.ipv6);
        meta.tcp_length = hdr.ipv6.payload_length;
        transition select(hdr.ipv6.next_header) {
            PROTO_TCP: tcp;
            PROTO_UDP: udp;
            PROTO_ICMP6: icmp6;
            default: accept;
        }
    }
    state tcp {
        packet.extract(hdr.tcp);
        transition accept;
    }
    state udp {
        packet.extract(hdr.udp);
        transition accept;
    }
    state icmp6 {
        packet.extract(hdr.icmp6);
        transition accept;
    }
    state icmp {
        packet.extract(hdr.icmp);
        transition accept;
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.cpu);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.ipv6);
        packet.emit(hdr.tcp);
        packet.emit(hdr.udp);
        packet.emit(hdr.icmp);
        packet.emit(hdr.icmp6);
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum_with_payload(meta.do_cksum == 1, { hdr.ipv6.src_addr, hdr.ipv6.dst_addr, (bit<32>)hdr.ipv6.payload_length, 24w0, PROTO_ICMP6 }, hdr.icmp6.checksum, HashAlgorithm.csum16);
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action drop() {
        mark_to_drop(standard_metadata);
    }
    action set_egress_port(port_t out_port) {
        standard_metadata.egress_spec = out_port;
    }
    action controller_debug() {
        meta.task = TASK_DEBUG;
        meta.ingress_port = standard_metadata.ingress_port;
        clone3(CloneType.I2E, 100, meta);
    }
    action controller_reply(task_t task) {
        meta.task = task;
        meta.ingress_port = standard_metadata.ingress_port;
        clone3(CloneType.I2E, 100, meta);
    }
    action multicast_pkg(mcast_t mcast_grp) {
        standard_metadata.mcast_grp = mcast_grp;
    }
    action icmp6_neighbor_solicitation(ipv6_addr_t addr) {
        standard_metadata.egress_spec = standard_metadata.ingress_port;
        hdr.ipv6.dst_addr = hdr.ipv6.src_addr;
        hdr.ipv6.src_addr = addr;
        hdr.icmp6.type = ICMP6_NA;
    }
    action icmp6_echo_reply() {
        mac_addr_t mac_tmp = hdr.ethernet.dst_addr;
        hdr.ethernet.dst_addr = hdr.ethernet.src_addr;
        hdr.ethernet.src_addr = mac_tmp;
        ipv6_addr_t addr_tmp = hdr.ipv6.dst_addr;
        hdr.ipv6.dst_addr = hdr.ipv6.src_addr;
        hdr.ipv6.src_addr = addr_tmp;
        hdr.icmp6.type = ICMP6_ECHO_REPLY;
        meta.do_cksum = 1;
    }
    table ndp_answer {
        key = {
            hdr.ipv6.dst_addr: exact;
            hdr.icmp6.type   : exact;
        }
        actions = {
            controller_debug;
            icmp6_neighbor_solicitation;
            NoAction;
        }
        size = 64;
        default_action = NoAction;
    }
    table port2mcast {
        key = {
            standard_metadata.ingress_port: exact;
        }
        actions = {
            multicast_pkg;
            controller_debug;
            NoAction;
        }
        size = 64;
        default_action = NoAction;
    }
    table addr2mcast {
        key = {
            hdr.ipv6.dst_addr: exact;
        }
        actions = {
            multicast_pkg;
            controller_debug;
            NoAction;
        }
        size = 64;
        default_action = NoAction;
    }
    table ndp {
        key = {
            hdr.ipv6.dst_addr             : lpm;
            standard_metadata.ingress_port: exact;
        }
        actions = {
            multicast_pkg;
            controller_debug;
            NoAction;
        }
        size = 64;
        default_action = controller_debug;
    }
    action icmp6_answer() {
        if (hdr.icmp6.isValid()) {
            if (hdr.icmp6.code == ICMP6_ECHO_REQUEST) {
                ipv6_addr_t tmp = hdr.ipv6.src_addr;
                hdr.ipv6.src_addr = hdr.ipv6.dst_addr;
                hdr.ipv6.dst_addr = tmp;
                hdr.icmp6.code = ICMP6_ECHO_REPLY;
            }
        }
    }
    table v6_addresses {
        key = {
            hdr.ipv6.dst_addr: exact;
        }
        actions = {
            controller_debug;
            controller_reply;
            icmp6_echo_reply;
            NoAction;
        }
        size = 64;
        default_action = NoAction;
    }
    table v6_networks {
        key = {
            hdr.ipv6.dst_addr: lpm;
        }
        actions = {
            set_egress_port;
            controller_debug;
            controller_reply;
            NoAction;
        }
        size = 64;
        default_action = NoAction;
    }
    table v4_networks {
        key = {
            hdr.ipv4.dst_addr: lpm;
        }
        actions = {
            set_egress_port;
            NoAction;
        }
        size = 64;
        default_action = NoAction;
    }
    apply {
        if (hdr.ipv6.isValid()) {
            v6_addresses.apply();
            v6_networks.apply();
        }
        if (hdr.ipv4.isValid()) {
            v4_networks.apply();
        }
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        if (standard_metadata.instance_type == 1) {
            hdr.cpu.setValid();
            hdr.cpu.task = meta.task;
            hdr.cpu.ethertype = hdr.ethernet.ethertype;
            hdr.cpu.ingress_port = (bit<16>)meta.ingress_port;
            hdr.ethernet.ethertype = TYPE_CPU;
        }
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

