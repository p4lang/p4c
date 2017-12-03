#include <core.p4>
#include <v1model.p4>

struct routing_metadata_t {
    bit<1> drop;
}

@name("ipv4_t") struct ipv4_t_1 {
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

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header icmp_t {
    bit<8>  hdr_type;
    bit<8>  code;
    bit<16> hdrChecksum;
}

header icmpv6_t {
    bit<8>  hdr_type;
    bit<8>  code;
    bit<16> hdrChecksum;
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

header ipv6_t {
    bit<4>   version;
    bit<8>   trafficClass;
    bit<20>  flowLabel;
    bit<16>  payloadLen;
    bit<8>   nextHdr;
    bit<8>   hopLimit;
    bit<128> srcAddr;
    bit<128> dstAddr;
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
    bit<16> hdr_length;
    bit<16> checksum;
}

header vlan_tag_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> etherType;
}

struct metadata {
    @name(".routing_metadata") 
    routing_metadata_t routing_metadata;
}

struct headers {
    @name(".ethernet") 
    ethernet_t    ethernet;
    @name(".icmp") 
    icmp_t        icmp;
    @name(".icmpv6") 
    icmpv6_t      icmpv6;
    @name(".ipv4") 
    ipv4_t        ipv4;
    @name(".ipv6") 
    ipv6_t        ipv6;
    @name(".tcp") 
    tcp_t         tcp;
    @name(".udp") 
    udp_t         udp;
    @name(".vlan_tag_") 
    vlan_tag_t[4] vlan_tag_;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x8100: parse_vlan;
            16w0x9100: parse_vlan;
            16w0x9200: parse_vlan;
            16w0x9300: parse_vlan;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
        }
    }
    @name(".parse_icmp") state parse_icmp {
        packet.extract<icmp_t>(hdr.icmp);
        transition accept;
    }
    @name(".parse_icmpv6") state parse_icmpv6 {
        packet.extract<icmpv6_t>(hdr.icmpv6);
        transition accept;
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.fragOffset, hdr.ipv4.protocol) {
            (13w0, 8w1): parse_icmp;
            (13w0, 8w6): parse_tcp;
            (13w0, 8w17): parse_udp;
        }
    }
    @name(".parse_ipv6") state parse_ipv6 {
        packet.extract<ipv6_t>(hdr.ipv6);
        transition select(hdr.ipv6.nextHdr) {
            8w58: parse_icmpv6;
            8w6: parse_tcp;
            8w17: parse_udp;
        }
    }
    @name(".parse_tcp") state parse_tcp {
        packet.extract<tcp_t>(hdr.tcp);
        transition accept;
    }
    @name(".parse_udp") state parse_udp {
        packet.extract<udp_t>(hdr.udp);
        transition accept;
    }
    @name(".parse_vlan") state parse_vlan {
        packet.extract<vlan_tag_t>(hdr.vlan_tag_.next);
        transition select(hdr.vlan_tag_.last.etherType) {
            16w0x8100: parse_vlan;
            16w0x9100: parse_vlan;
            16w0x9200: parse_vlan;
            16w0x9300: parse_vlan;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
        }
    }
    @name(".start") state start {
        meta.routing_metadata.drop = 1w0;
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".cnt1") counter(32w32, CounterType.packets) cnt1;
    @name(".drop_pkt") action drop_pkt() {
        mark_to_drop();
    }
    @name(".hop") action hop(inout bit<8> ttl, bit<9> egress_spec) {
        ttl = ttl + 8w255;
        standard_metadata.egress_spec[8:0] = egress_spec[8:0];
    }
    @name(".hop_ipv4") action hop_ipv4(bit<9> egress_spec) {
        hop(hdr.ipv4.ttl, egress_spec);
    }
    @name(".act") action act() {
        cnt1.count(32w10);
    }
    @name(".ipv4_routing") table ipv4_routing {
        actions = {
            drop_pkt();
            hop_ipv4();
            @defaultonly NoAction();
        }
        key = {
            hdr.ipv4.dstAddr: lpm @name("ipv4.dstAddr") ;
        }
        default_action = NoAction();
    }
    @name(".table_2") table table_2 {
        actions = {
            act();
            @defaultonly NoAction();
        }
        key = {
            hdr.ipv4.dstAddr: lpm @name("ipv4.dstAddr") ;
        }
        default_action = NoAction();
    }
    apply {
        ipv4_routing.apply();
        table_2.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<vlan_tag_t[4]>(hdr.vlan_tag_);
        packet.emit<ipv6_t>(hdr.ipv6);
        packet.emit<icmpv6_t>(hdr.icmpv6);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<udp_t>(hdr.udp);
        packet.emit<tcp_t>(hdr.tcp);
        packet.emit<icmp_t>(hdr.icmp);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

