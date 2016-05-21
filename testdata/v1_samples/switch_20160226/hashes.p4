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

/*****************************************************************************/
/* HASH calculation                                                          */
/*****************************************************************************/
header_type hash_metadata_t {
    fields {
        hash1 : 16;
        hash2 : 16;
        entropy_hash : 16;
    }
}

@pragma pa_atomic ingress hash_metadata.hash1
@pragma pa_solitary ingress hash_metadata.hash1
@pragma pa_atomic ingress hash_metadata.hash2
@pragma pa_solitary ingress hash_metadata.hash2
metadata hash_metadata_t hash_metadata;

field_list lkp_ipv4_hash1_fields {
    ipv4_metadata.lkp_ipv4_sa;
    ipv4_metadata.lkp_ipv4_da;
    l3_metadata.lkp_ip_proto;
    l3_metadata.lkp_l4_sport;
    l3_metadata.lkp_l4_dport;
}

field_list lkp_ipv4_hash2_fields {
    l2_metadata.lkp_mac_sa;
    l2_metadata.lkp_mac_da;
    ipv4_metadata.lkp_ipv4_sa;
    ipv4_metadata.lkp_ipv4_da;
    l3_metadata.lkp_ip_proto;
    l3_metadata.lkp_l4_sport;
    l3_metadata.lkp_l4_dport;
}

field_list_calculation lkp_ipv4_hash1 {
    input {
        lkp_ipv4_hash1_fields;
    }
    algorithm : crc16;
    output_width : 16;
}

field_list_calculation lkp_ipv4_hash2 {
    input {
        lkp_ipv4_hash2_fields;
    }
    algorithm : crc16;
    output_width : 16;
}

action compute_lkp_ipv4_hash() {
    modify_field_with_hash_based_offset(hash_metadata.hash1, 0,
                                        lkp_ipv4_hash1, 65536);
    modify_field_with_hash_based_offset(hash_metadata.hash2, 0,
                                        lkp_ipv4_hash2, 65536);
}

field_list lkp_ipv6_hash1_fields {
    ipv6_metadata.lkp_ipv6_sa;
    ipv6_metadata.lkp_ipv6_da;
    l3_metadata.lkp_ip_proto;
    l3_metadata.lkp_l4_sport;
    l3_metadata.lkp_l4_dport;
}

field_list lkp_ipv6_hash2_fields {
    l2_metadata.lkp_mac_sa;
    l2_metadata.lkp_mac_da;
    ipv6_metadata.lkp_ipv6_sa;
    ipv6_metadata.lkp_ipv6_da;
    l3_metadata.lkp_ip_proto;
    l3_metadata.lkp_l4_sport;
    l3_metadata.lkp_l4_dport;
}

field_list_calculation lkp_ipv6_hash1 {
    input {
        lkp_ipv6_hash1_fields;
    }
    algorithm : crc16;
    output_width : 16;
}

field_list_calculation lkp_ipv6_hash2 {
    input {
        lkp_ipv6_hash2_fields;
    }
    algorithm : crc16;
    output_width : 16;
}

action compute_lkp_ipv6_hash() {

    modify_field_with_hash_based_offset(hash_metadata.hash1, 0,
                                        lkp_ipv6_hash1, 65536);

    modify_field_with_hash_based_offset(hash_metadata.hash2, 0,
                                        lkp_ipv6_hash2, 65536);
}

field_list lkp_non_ip_hash2_fields {
    ingress_metadata.ifindex;
    l2_metadata.lkp_mac_sa;
    l2_metadata.lkp_mac_da;
    l2_metadata.lkp_mac_type;
}

field_list_calculation lkp_non_ip_hash2 {
    input {
        lkp_non_ip_hash2_fields;
    }
    algorithm : crc16;
    output_width : 16;
}

action compute_lkp_non_ip_hash() {
    modify_field_with_hash_based_offset(hash_metadata.hash2, 0,
                                        lkp_non_ip_hash2, 65536);
}

table compute_ipv4_hashes {
    reads {
        ingress_metadata.drop_flag : exact;
    }
    actions {
        compute_lkp_ipv4_hash;
    }
}

table compute_ipv6_hashes {
    reads {
        ingress_metadata.drop_flag : exact;
    }
    actions {
        compute_lkp_ipv6_hash;
    }
}

table compute_non_ip_hashes {
    reads {
        ingress_metadata.drop_flag : exact;
    }
    actions {
        compute_lkp_non_ip_hash;
    }
}

action computed_two_hashes() {
    modify_field(intrinsic_metadata.mcast_hash, hash_metadata.hash1);
    modify_field(hash_metadata.entropy_hash, hash_metadata.hash2);
}

action computed_one_hash() {
    modify_field(hash_metadata.hash1, hash_metadata.hash2);
    modify_field(intrinsic_metadata.mcast_hash, hash_metadata.hash2);
    modify_field(hash_metadata.entropy_hash, hash_metadata.hash2);
}

table compute_other_hashes {
    reads {
        hash_metadata.hash1 : exact;
    }
    actions {
        computed_two_hashes;
        computed_one_hash;
    }
}

control process_hashes {
    if (((tunnel_metadata.tunnel_terminate == FALSE) and valid(ipv4)) or
        ((tunnel_metadata.tunnel_terminate == TRUE) and valid(inner_ipv4))) {
        apply(compute_ipv4_hashes);
    }
#ifndef IPV6_DISABLE
    else {
        if (((tunnel_metadata.tunnel_terminate == FALSE) and valid(ipv6)) or
             ((tunnel_metadata.tunnel_terminate == TRUE) and valid(inner_ipv6))) {
            apply(compute_ipv6_hashes);
        }
#endif /* IPV6_DISABLE */
        else {
            apply(compute_non_ip_hashes);
        }
#ifndef IPV6_DISABLE
    }
#endif /* IPV6_DISABLE */
    apply(compute_other_hashes);
}
