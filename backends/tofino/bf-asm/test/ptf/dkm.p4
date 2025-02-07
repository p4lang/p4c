#include <tofino/constants.p4>
#include <tofino/intrinsic_metadata.p4>
#include <tofino/primitives.p4>

#define VLAN_DEPTH             2
#define ETHERTYPE_VLAN         0x8100
#define ETHERTYPE_IPV4         0x0800


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
    dstAddr : 32;
  }
}

header_type tcp_t {
  fields {
    srcPort : 16;
    dstPort : 16;
    seqNo   : 32;
    ackNo   : 32;
    dataOffset : 4;
    res     : 3;
    ecn     : 3;
    ctrl    :3;
    window  : 16;
    checksum : 16;
    urgentPtr : 16;
  }
}

header_type metadata_t {
  fields {
    table_hit : 1;
    table_id  : 8;
  }
}

header ethernet_t ethernet;
header vlan_tag_t vlan_tag;
header ipv4_t ipv4;
header tcp_t tcp;
metadata metadata_t md;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0x8100 : parse_vlan;
        default : ingress;
    }
}

parser parse_vlan {
    extract(vlan_tag);
    return select(latest.etherType) {
      0x800 : parse_ipv4;
      default: ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return select (latest.protocol) {
      6 : parse_tcp;
      default: ingress;
    }
}

parser parse_tcp {
    extract(tcp);
    return ingress;
}

action switch_to_dest_port (dport) {
  modify_field(md.table_hit, 1);
  modify_field(ig_intr_md_for_tm.ucast_egress_port, dport);
}

action switch_to_miss_port(miss_port) {
  modify_field(ig_intr_md_for_tm.ucast_egress_port, miss_port);
}

action nop() {
}


/*
 * Exm tables covering following scenarios are implemented/Tested using this p4.
 *  - Multiple match spec packed in one sram RAM word.
 *  - Multiple match spec packed in more than one sram RAM word.
 *  - More than 1 way.
 *  - Table spaning more than 1 stage.
 */



@pragma stage 0
@pragma ways 3
@pragma pack 2
@pragma dynamic_table_key_masks 1
table l2_stage0_ways_3_pack_2 {
    reads {
      ethernet.srcAddr : exact;
      ethernet.dstAddr : exact;
    }
    actions {
      switch_to_dest_port;
    }
    size : 1024;
}


@pragma stage 1
@pragma ways 6
@pragma pack 2
@pragma dynamic_table_key_masks 1
table l2_stage1_ways_6_pack_2 {
    reads {
      ethernet.srcAddr : exact;
      ethernet.dstAddr : exact;
    }
    actions {
      switch_to_dest_port;
    }
    size : 1024;
}


@pragma stage 2
@pragma ways 6
@pragma pack 3
@pragma dynamic_table_key_masks 1
table l2_stage2_ways_6_pack_3 {
    reads {
      ethernet.srcAddr : exact;
      ethernet.dstAddr : exact;
    }
    actions {
      switch_to_dest_port;
    }
    size : 1024;
}

@pragma ways 6
@pragma pack 3
@pragma dynamic_table_key_masks 1
table l2_multistage_ways_6_pack_3 {
    reads {
      ethernet.srcAddr : exact;
      ethernet.dstAddr : exact;
    }
    actions {
      switch_to_dest_port;
    }
    size: 64000;
}



table miss_check {
  reads {
    md.table_hit : exact;
  }
  actions {
    switch_to_miss_port;
  }
  size : 1;
}



control ingress {
    apply(l2_stage0_ways_3_pack_2);
    apply(l2_stage1_ways_6_pack_2);
    apply(l2_stage2_ways_6_pack_3);
    apply(l2_multistage_ways_6_pack_3);
    apply(miss_check);
}

control egress {
}
