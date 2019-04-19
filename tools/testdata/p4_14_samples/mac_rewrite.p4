/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

parser start {
    return parse_ethernet;
}

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header ethernet_t ethernet;
#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_IPV6 0x86dd

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        ETHERTYPE_IPV6 : parse_ipv6;
        default: ingress;
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
header ipv4_t ipv4;
parser parse_ipv4 {
    extract(ipv4);
    return ingress;
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
header ipv6_t ipv6;

parser parse_ipv6 {
    extract(ipv6);
    return ingress;
}

header_type egress_metadata_t {
    fields {
        payload_length : 16;                   /* payload length for tunnels */
        smac_idx : 9;                          /* index into source mac table */
        bd : 16;                               /* egress inner bd */
        inner_replica : 1;                     /* is copy is due to inner replication */
        replica : 1;                           /* is this a replica */
        mac_da : 48;                           /* final mac da */
        routed : 1;                            /* is this replica routed */
        same_bd_check : 16;                    /* ingress bd xor egress bd */

        header_count: 4;                       /* Number of headers */

        drop_reason : 8;                       /* drop reason for negative mirroring */
        egress_bypass : 1;                     /* skip the entire egress pipeline */
        drop_exception : 8;                    /* MTU check fail, .. */
    }
}
metadata egress_metadata_t egress_metadata;

action do_setup(idx, routed) {
    modify_field(egress_metadata.mac_da, ethernet.dstAddr);
    modify_field(egress_metadata.smac_idx, idx);
    modify_field(egress_metadata.routed, routed);
}

table setup {
    reads { ethernet: valid; }
    actions { do_setup; }
}

control ingress {
    apply(setup);
    process_mac_rewrite();
}

action nop() { }

action rewrite_ipv4_unicast_mac(smac) {
    modify_field(ethernet.srcAddr, smac);
    modify_field(ethernet.dstAddr, egress_metadata.mac_da);
    add_to_field(ipv4.ttl, -1);
}

action rewrite_ipv4_multicast_mac(smac) {
    modify_field(ethernet.srcAddr, smac);
    modify_field(ethernet.dstAddr, 0x01005E000000, 0xFFFFFF800000);
    add_to_field(ipv4.ttl, -1);
}

action rewrite_ipv6_unicast_mac(smac) {
    modify_field(ethernet.srcAddr, smac);
    modify_field(ethernet.dstAddr, egress_metadata.mac_da);
    add_to_field(ipv6.hopLimit, -1);
}

action rewrite_ipv6_multicast_mac(smac) {
    modify_field(ethernet.srcAddr, smac);
    modify_field(ethernet.dstAddr, 0x333300000000, 0xFFFF00000000);
    add_to_field(ipv6.hopLimit, -1);
}

table mac_rewrite {
    reads {
        egress_metadata.smac_idx : exact;
        ipv4 : valid;
        ipv6 : valid;
    }
    actions {
        nop;
        rewrite_ipv4_unicast_mac;
        rewrite_ipv4_multicast_mac;
        rewrite_ipv6_unicast_mac;
        rewrite_ipv6_multicast_mac;
    }
    size : 512;
}

control process_mac_rewrite {
    if (egress_metadata.routed == 1) {
        apply(mac_rewrite);
    }
}
