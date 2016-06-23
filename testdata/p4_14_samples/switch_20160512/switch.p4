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

#include "includes/p4features.h"
#include "includes/drop_reasons.h"
#include "includes/headers.p4"
#include "includes/parser.p4"
#include "includes/sizes.p4"
#include "includes/defines.p4"
#include "includes/intrinsic.p4"
#include "archdeps.p4"

/* METADATA */
header_type ingress_metadata_t {
    fields {
        ingress_port : 9;                      /* input physical port */
        ifindex : IFINDEX_BIT_WIDTH;           /* input interface index */
        egress_ifindex : IFINDEX_BIT_WIDTH;    /* egress interface index */
        port_type : 2;                         /* ingress port type */

        outer_bd : BD_BIT_WIDTH;               /* outer BD */
        bd : BD_BIT_WIDTH;                     /* BD */

        drop_flag : 1;                         /* if set, drop the packet */
        drop_reason : 8;                       /* drop reason */
        control_frame: 1;                      /* control frame */
        bypass_lookups : 16;                   /* list of lookups to skip */
        sflow_take_sample : 32 (saturating);
    }
}

header_type egress_metadata_t {
    fields {
        bypass : 1;                            /* bypass egress pipeline */
        port_type : 2;                         /* egress port type */
        payload_length : 16;                   /* payload length for tunnels */
        smac_idx : 9;                          /* index into source mac table */
        bd : BD_BIT_WIDTH;                     /* egress inner bd */
        outer_bd : BD_BIT_WIDTH;               /* egress inner bd */
        mac_da : 48;                           /* final mac da */
        routed : 1;                            /* is this replica routed */
        same_bd_check : BD_BIT_WIDTH;          /* ingress bd xor egress bd */
        drop_reason : 8;                       /* drop reason */
        ifindex : IFINDEX_BIT_WIDTH;           /* egress interface index */
    }
}

metadata ingress_metadata_t ingress_metadata;
metadata egress_metadata_t egress_metadata;

/* Global config information */
header_type global_config_metadata_t {
    fields {
        enable_dod : 1;                        /* Enable Deflection-on-Drop */
        /* Add more global parameters such as switch_id.. */
    }
}
metadata global_config_metadata_t global_config_metadata;

#include "switch_config.p4"
#ifdef OPENFLOW_ENABLE
#include "openflow.p4"
#endif /* OPENFLOW_ENABLE */
#include "port.p4"
#include "l2.p4"
#include "l3.p4"
#include "ipv4.p4"
#include "ipv6.p4"
#include "tunnel.p4"
#include "acl.p4"
#include "multicast.p4"
#include "nexthop.p4"
#include "rewrite.p4"
#include "security.p4"
#include "fabric.p4"
#include "egress_filter.p4"
#include "mirror.p4"
#include "int_transit.p4"
#include "hashes.p4"
#include "meter.p4"
#include "sflow.p4"

action nop() {
}

action on_miss() {
}

control ingress {
    /* input mapping - derive an ifindex */
    process_ingress_port_mapping();

    /* process outer packet headers */
    process_validate_outer_header();

    /* read and apply system confuration parametes */
    process_global_params();

#ifdef OPENFLOW_ENABLE
    if (ingress_metadata.port_type == PORT_TYPE_CPU) {
        apply(packet_out);
    }
#endif /* OPENFLOW_ENABLE */

    /* derive bd and its properties  */
    process_port_vlan_mapping();

    /* spanning tree state checks */
    process_spanning_tree();

    /* IPSG */
    process_ip_sourceguard();

    /* INT src,sink determination */
    process_int_endpoint();

    /* tunnel termination processing */
    process_tunnel();

    /* ingress sflow determination */
    process_ingress_sflow();

    /* storm control */
    process_storm_control();

    if (ingress_metadata.port_type != PORT_TYPE_FABRIC) {
#ifndef MPLS_DISABLE
        if (not (valid(mpls[0]) and (l3_metadata.fib_hit == TRUE))) {
#endif /* MPLS_DISABLE */

            /* validate packet */
            process_validate_packet();

            /* l2 lookups */
            process_mac();

            /* port and vlan ACL */
            if (l3_metadata.lkp_ip_type == IPTYPE_NONE) {
                process_mac_acl();
            } else {
                process_ip_acl();
            }

            process_qos();

            apply(rmac) {
                rmac_miss {
                    process_multicast();
                }
                default {
                    if (DO_LOOKUP(L3)) {
                        if ((l3_metadata.lkp_ip_type == IPTYPE_IPV4) and
                            (ipv4_metadata.ipv4_unicast_enabled == TRUE)) {
                            /* router ACL/PBR */
                            process_ipv4_racl();

                            process_ipv4_urpf();
                            process_ipv4_fib();

                        } else {
                            if ((l3_metadata.lkp_ip_type == IPTYPE_IPV6) and
                                (ipv6_metadata.ipv6_unicast_enabled == TRUE)) {

                                /* router ACL/PBR */
                                process_ipv6_racl();
                                process_ipv6_urpf();
                                process_ipv6_fib();
                            }
                        }
                        process_urpf_bd();
                    }
                }
            }
#ifndef MPLS_DISABLE
        }
#endif /* MPLS_DISABLE */
    }

    process_meter_index();

    /* compute hashes based on packet type  */
    process_hashes();

    process_meter_action();

    if (ingress_metadata.port_type != PORT_TYPE_FABRIC) {
        /* update statistics */
        process_ingress_bd_stats();
        process_ingress_acl_stats();
        process_storm_control_stats();

        /* decide final forwarding choice */
        process_fwd_results();

        /* ecmp/nexthop lookup */
        process_nexthop();

        if (ingress_metadata.egress_ifindex == IFINDEX_FLOOD) {
            /* resolve multicast index for flooding */
            process_multicast_flooding();
        } else {
            /* resolve final egress port for unicast traffic */
            process_lag();
        }

#ifdef OPENFLOW_ENABLE
        /* openflow processing for ingress */
        process_ofpat_ingress();
#endif /* OPENFLOW_ENABLE */

        /* generate learn notify digest if permitted */
        process_mac_learning();
    }

    /* resolve fabric port to destination device */
    process_fabric_lag();

    if (ingress_metadata.port_type != PORT_TYPE_FABRIC) {
        /* system acls */
        process_system_acl();
    }
}

control egress {

#ifdef OPENFLOW_ENABLE
    if (openflow_metadata.ofvalid == TRUE) {
        process_ofpat_egress();
    } else {
#endif /* OPENFLOW_ENABLE */
        /* check for -ve mirrored pkt */
        if ((intrinsic_metadata.deflection_flag == FALSE) and
            (egress_metadata.bypass == FALSE)) {

            /* check if pkt is mirrored */
            if (pkt_is_mirrored) {
                /* set the nexthop for the mirror id */
                /* for sflow i2e mirror pkt, result will set required sflow info */
                apply(mirror);
            } else {

                /* multi-destination replication */
                process_replication();
            }

            /* determine egress port properties */
            apply(egress_port_mapping) {
                egress_port_type_normal {
                    if (pkt_is_not_mirrored) {
                        /* strip vlan header */
                        process_vlan_decap();
                    }

                    /* perform tunnel decap */
                    process_tunnel_decap();


                    /* apply nexthop_index based packet rewrites */
                    process_rewrite();

                    /* egress bd properties */
                    process_egress_bd();

                    /* rewrite source/destination mac if needed */
                    process_mac_rewrite();

                    /* egress mtu checks */
                    process_mtu();

                    /* INT processing */
                    process_int_insertion();

                    process_egress_bd_stats();
                }
            }
    
            /* perform tunnel encap */
            process_tunnel_encap();

            /* update underlay header based on INT information inserted */
            process_int_outer_encap();

            if (egress_metadata.port_type == PORT_TYPE_NORMAL) {
                /* egress vlan translation */
                process_vlan_xlate();
            }

            /* egress filter */
            process_egress_filter();
        }

        /* apply egress acl */
        process_egress_acl();
#ifdef OPENFLOW_ENABLE
    }
#endif /* OPENFLOW_ENABLE */
}
