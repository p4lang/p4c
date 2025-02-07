#include <tofino/intrinsic_metadata.p4>

#define _parser_counter_ ig_prsr_ctrl.parser_counter

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_IPV6 0x86dd

#define IP_PROTOCOLS_SR 43
#define SRH_MAX_SEGMENTS 9

#define IPV4_OPTION_EOL_VALUE 0x00
#define IPV4_OPTION_NOP_VALUE 0x01
#define IPV4_OPTION_ADDRESS_EXTENSION 0x93

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
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

// End of Option List
header_type ipv4_option_EOL_t {
    fields {
        value : 8;
    }
}

// No operation
header_type ipv4_option_NOP_t {
    fields {
        value : 8;
    }
}

header_type ipv4_option_addr_ext_t {
    fields {
        value : 8;
        len : 8;
        src_ext : 32;
        dst_ext : 32;
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

header_type ipv6_srh_t {
    fields {
        nextHdr : 8;
        hdrExtLen : 8;
        routingType : 8;
        segLeft : 8;
        firstSeg : 8;
        flags : 16;
        reserved : 8;
    }
}

header_type ipv6_srh_segment_t {
    fields {
        sid : 128;
    }
}

header ethernet_t ethernet;
header ipv4_t ipv4;
@pragma header_ordering  ipv4_option_addr_ext ipv4_option_NOP ipv4_option_EOL
header ipv4_option_EOL_t ipv4_option_EOL;
header ipv4_option_NOP_t ipv4_option_NOP;
header ipv4_option_addr_ext_t ipv4_option_addr_ext;
header ipv6_t ipv6;
header ipv6_srh_t ipv6_srh;
header ipv6_srh_segment_t ipv6_srh_seg_list[SRH_MAX_SEGMENTS];
header ipv6_srh_segment_t active_segment;


parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        ETHERTYPE_IPV6 : parse_ipv6;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    // Little bit hacky. This shouldve been ipv4.ihl * 4 - 20. Extra -1 is to
    // get rid of the effect of version (0x0100)
    set_metadata(_parser_counter_, ipv4.ihl * 4 - 21);
    return select(ipv4.ihl) {
        0x05 : ingress;
        default : parse_ipv4_options;
    }
}

parser parse_ipv4_options {
    // match on byte counter and option value
    return select(_parser_counter_, current(0, 8)) {
        0x0000 mask 0xff00 : ingress;
        0x0000 mask 0x00ff : parse_ipv4_option_eol;
        0x0001 mask 0x00ff : parse_ipv4_option_nop;
        0x0093 mask 0x00ff : parse_ipv4_option_addr_ext;
    }
}

parser parse_ipv4_option_eol {
    extract(ipv4_option_EOL);
    set_metadata(_parser_counter_, _parser_counter_ - 1);
    return parse_ipv4_options;
}

parser parse_ipv4_option_nop {
    extract(ipv4_option_NOP);
    set_metadata(_parser_counter_, _parser_counter_ - 1);
    return parse_ipv4_options;
}

parser parse_ipv4_option_addr_ext {
    extract(ipv4_option_addr_ext);
    // security option must have length 11 bytes
    set_metadata(_parser_counter_, _parser_counter_ - 10);
    return parse_ipv4_options;
}

parser parse_ipv6 {
    extract(ipv6);
    return select(latest.nextHdr) {
        IP_PROTOCOLS_SR : parse_ipv6_srh;
    }
}

parser parse_ipv6_srh {
    extract(ipv6_srh);
    set_metadata(_parser_counter_, ipv6_srh.segLeft);
    return parse_ipv6_srh_seg_list;
}

parser parse_ipv6_srh_seg_list {
    return select(_parser_counter_) {
        0x00 : parse_ipv6_srh_active_segment;
        default : parse_ipv6_srh_segment;
    }
}

parser parse_ipv6_srh_segment {
    extract(ipv6_srh_seg_list[next]);
    set_metadata(_parser_counter_, _parser_counter_ - 1);
    return parse_ipv6_srh_seg_list;
}

parser parse_ipv6_srh_active_segment {
    extract(active_segment);
    return ingress;
}

action rewrite_ipv6_and_set_egress_port() {
    modify_field(ipv6.dstAddr, active_segment.sid);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 1);
}

action set_egress_port() {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 1);
}

table sr_lookup {
    actions {
        rewrite_ipv6_and_set_egress_port;
    }
}

table ipv4_option_lookup {
    reads {
        ipv4_option_NOP : valid;
        ipv4_option_EOL : valid;
        ipv4_option_addr_ext : valid;
    }
    actions {
        set_egress_port;
    }
}

control ingress {
    if (valid(ipv6_srh)) {
        apply(sr_lookup);
    }
    if (valid(ipv4)) {
        apply(ipv4_option_lookup);
    }
}

control egress {

}
