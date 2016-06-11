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

/*
 * IPv4 processing
 */

/*
 * IPv4 metadata
 */
header_type ipv4_metadata_t {
    fields {
        lkp_ipv4_sa : 32;
        lkp_ipv4_da : 32;
        ipv4_unicast_enabled : 1;      /* is ipv4 unicast routing enabled */
        ipv4_urpf_mode : 2;            /* 0: none, 1: strict, 3: loose */
    }
}

metadata ipv4_metadata_t ipv4_metadata;

#if !defined(L3_DISABLE) && !defined(IPV4_DISABLE)
/*****************************************************************************/
/* Validate outer IPv4 header                                                */
/*****************************************************************************/
action set_valid_outer_ipv4_packet() {
    modify_field(l3_metadata.lkp_ip_type, IPTYPE_IPV4);
    modify_field(l3_metadata.lkp_ip_tc, ipv4.diffserv);
    modify_field(l3_metadata.lkp_ip_version, 4);
}

action set_malformed_outer_ipv4_packet(drop_reason) {
    modify_field(ingress_metadata.drop_flag, TRUE);
    modify_field(ingress_metadata.drop_reason, drop_reason);
}

table validate_outer_ipv4_packet {
    reads {
        ipv4.version : ternary;
        l3_metadata.lkp_ip_ttl : ternary;
#ifndef __TARGET_BMV2__
        ipv4_metadata.lkp_ipv4_sa mask 0xFF000000 : ternary;
#else
        ipv4_metadata.lkp_ipv4_sa : ternary;
#endif
    }
    actions {
        set_valid_outer_ipv4_packet;
        set_malformed_outer_ipv4_packet;
    }
    size : VALIDATE_PACKET_TABLE_SIZE;
}
#endif /* L3_DISABLE && IPV4_DISABLE */

control validate_outer_ipv4_header {
#if !defined(L3_DISABLE) && !defined(IPV4_DISABLE)
    apply(validate_outer_ipv4_packet);
#endif /* L3_DISABLE && IPV4_DISABLE */
}

#if !defined(L3_DISABLE) && !defined(IPV4_DISABLE)
/*****************************************************************************/
/* IPv4 FIB lookup                                                           */
/*****************************************************************************/
table ipv4_fib {
    reads {
        l3_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da : exact;
    }
    actions {
        on_miss;
        fib_hit_nexthop;
        fib_hit_ecmp;
    }
    size : IPV4_HOST_TABLE_SIZE;
}

table ipv4_fib_lpm {
    reads {
        l3_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da : lpm;
    }
    actions {
        on_miss;
        fib_hit_nexthop;
        fib_hit_ecmp;
    }
    size : IPV4_LPM_TABLE_SIZE;
}
#endif /* L3_DISABLE && IPV4_DISABLE */

control process_ipv4_fib {
#if !defined(L3_DISABLE) && !defined(IPV4_DISABLE)
    /* fib lookup */
    apply(ipv4_fib) {
        on_miss {
            apply(ipv4_fib_lpm);
        }
    }
#endif /* L3_DISABLE && IPV4_DISABLE */
}

#if !defined(L3_DISABLE) && !defined(IPV4_DISABLE) && !defined(URPF_DISABLE)
/*****************************************************************************/
/* IPv4 uRPF lookup                                                          */
/*****************************************************************************/
action ipv4_urpf_hit(urpf_bd_group) {
    modify_field(l3_metadata.urpf_hit, TRUE);
    modify_field(l3_metadata.urpf_bd_group, urpf_bd_group);
    modify_field(l3_metadata.urpf_mode, ipv4_metadata.ipv4_urpf_mode);
}

table ipv4_urpf_lpm {
    reads {
        l3_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_sa : lpm;
    }
    actions {
        ipv4_urpf_hit;
        urpf_miss;
    }
    size : IPV4_LPM_TABLE_SIZE;
}

table ipv4_urpf {
    reads {
        l3_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_sa : exact;
    }
    actions {
        on_miss;
        ipv4_urpf_hit;
    }
    size : IPV4_HOST_TABLE_SIZE;
}
#endif /* L3_DISABLE && IPV4_DISABLE && URPF_DISABLE */

control process_ipv4_urpf {
#if !defined(L3_DISABLE) && !defined(IPV4_DISABLE) && !defined(URPF_DISABLE)
    /* unicast rpf lookup */
    if (ipv4_metadata.ipv4_urpf_mode != URPF_MODE_NONE) {
        apply(ipv4_urpf) {
            on_miss {
                apply(ipv4_urpf_lpm);
            }
        }
    }
#endif /* L3_DISABLE && IPV4_DISABLE && URPF_DISABLE */
}
