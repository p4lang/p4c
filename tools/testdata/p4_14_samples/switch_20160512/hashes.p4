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
#ifdef INCLUDE_OLD_INTRINSIC_METADATA_FIELDS
    modify_field(intrinsic_metadata.mcast_hash, hash_metadata.hash1);
#endif // INCLUDE_OLD_INTRINSIC_METADATA_FIELDS
    modify_field(hash_metadata.entropy_hash, hash_metadata.hash2);
}

action computed_one_hash() {
    modify_field(hash_metadata.hash1, hash_metadata.hash2);
#ifdef INCLUDE_OLD_INTRINSIC_METADATA_FIELDS
    modify_field(intrinsic_metadata.mcast_hash, hash_metadata.hash2);
#endif // INCLUDE_OLD_INTRINSIC_METADATA_FIELDS
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
