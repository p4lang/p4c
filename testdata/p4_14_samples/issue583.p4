header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type vlan_tag_t {
    fields {
        pcp : 3;
        cfi : 1;
        vid : 12;
        etherType : 16;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header_type ipv6_t {
    fields {
        version : 4;
        trafficClass : 8;
        flowLabel : 20;
        payloadLen : 16;
        nextHdr : 8;
        hopLimit : 8;
        srcAddr : 128;
        dstAddr : 128;
    }
}

header_type icmp_t {
    fields {
        hdr_type : 8;
        code : 8;
        hdrChecksum : 16;
    }
}

header_type icmpv6_t {
    fields {
        hdr_type : 8;
        code : 8;
        hdrChecksum : 16;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 3;
        ecn : 3;
        ctrl : 6;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        hdr_length : 16;
        checksum : 16;
    }
}

header_type routing_metadata_t {
    fields {
        drop : 1;
    }
}

metadata routing_metadata_t routing_metadata;

parser start {
    set_metadata(routing_metadata.drop, 0);
    return parse_ethernet;
}

#define ETHERTYPE_VLAN 0x8100, 0x9100, 0x9200, 0x9300
#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_IPV6 0x86dd
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_RARP 0x8035

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_VLAN : parse_vlan;
        ETHERTYPE_IPV4 : parse_ipv4;
        ETHERTYPE_IPV6 : parse_ipv6;
    }
}

#define VLAN_DEPTH 4
header vlan_tag_t vlan_tag_[VLAN_DEPTH];

parser parse_vlan {
    extract(vlan_tag_[next]);
    return select(latest.etherType) {
        ETHERTYPE_VLAN : parse_vlan;
        ETHERTYPE_IPV4 : parse_ipv4;
        ETHERTYPE_IPV6 : parse_ipv6;
    }
}

#define IP_PROTOCOLS_ICMP 1
#define IP_PROTOCOLS_TCP 6
#define IP_PROTOCOLS_UDP 17
#define IP_PROTOCOLS_ICMPV6 58

header ipv4_t ipv4;

parser parse_ipv4 {
    extract(ipv4);
    return select(latest.fragOffset, latest.protocol) {
        IP_PROTOCOLS_ICMP : parse_icmp;
        IP_PROTOCOLS_TCP : parse_tcp;
        IP_PROTOCOLS_UDP : parse_udp;
    }
}

header ipv6_t ipv6;

parser parse_ipv6 {
    extract(ipv6);
    return select(latest.nextHdr) {
        IP_PROTOCOLS_ICMPV6 : parse_icmpv6;
        IP_PROTOCOLS_TCP : parse_tcp;
        IP_PROTOCOLS_UDP : parse_udp;
    }
}

header icmp_t icmp;

parser parse_icmp {
    extract(icmp);
    return ingress;
}

header icmpv6_t icmpv6;

parser parse_icmpv6 {
    extract(icmpv6);
    return ingress;
}

header tcp_t tcp;

parser parse_tcp {
    extract(tcp);
    return ingress;
}


header udp_t udp;

parser parse_udp {
    extract(udp);
    return ingress;
}

action hop(ttl, egress_spec) {
    add_to_field(ttl, -1);
    modify_field(standard_metadata.egress_spec, egress_spec, 0xFFFFFFFF);
}

action hop_ipv4(egress_spec) {
    hop(ipv4.ttl, egress_spec);
}

/* This should not be necessary if drop is allowed in table action specs */
action drop_pkt() {
    drop();
}

table ipv4_routing {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
      drop_pkt;
      hop_ipv4;
    }
}

action act() {
    count(cnt1, 10);
}

table table_2 {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
        act;
    }
}

counter cnt1 {
    type : packets;
    static : table_2;
    instance_count : 32;
}

register reg1 {
    width : 20;
    static : ipv4_routing;
    instance_count : 100;
    attributes : saturating, signed;
}

register reg2 {
    layout : ipv4_t;
    direct : ipv4_routing;
}


control ingress {
    apply(ipv4_routing);
    apply(table_2);
}

control egress {

}