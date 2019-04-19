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


header_type intrinsic_metadata_t {
    fields {
        exclusion_id1 : 16;
    }
}
metadata intrinsic_metadata_t intrinsic_metadata;

header_type ingress_metadata_t {
    fields {
        bd : 16;
        vrf : 12;
        ipv4_unicast_enabled : 1;              /* is ipv4 unicast routing enabled on BD */
        ipv6_unicast_enabled : 1;              /* is ipv6 unicast routing enabled on BD */
        ipv4_multicast_mode : 2;               /* ipv4 multicast mode BD */
        ipv6_multicast_mode : 2;               /* ipv6 multicast mode BD */
        igmp_snooping_enabled : 1;             /* is IGMP snooping enabled on BD */
        mld_snooping_enabled : 1;              /* is MLD snooping enabled on BD */
        ipv4_urpf_mode : 2;                    /* 0: none, 1: strict, 3: loose */
        ipv6_urpf_mode : 2;                    /* 0: none, 1: strict, 3: loose */
        rmac_group : 10;                       /* Rmac group, for rmac indirection */
        bd_mrpf_group : 16;                    /* rpf group from bd lookup */
        uuc_mc_index : 16;                     /* unknown unicast multicast index */
        umc_mc_index : 16;                     /* unknown multicast multicast index */
        bcast_mc_index : 16;                   /* broadcast multicast index */
        bd_label : 16;                         /* bd label for acls */
    }
}
metadata ingress_metadata_t ingress_metadata;

header_type data_t {
    fields {
        f1 : 16;
        f2 : 16;
    }
}
header data_t data;

parser start {
    extract(data);
    set_metadata(ingress_metadata.bd, data.f2);
    return ingress;
}

action set_bd_info(vrf, rmac_group, mrpf_group,
        bd_label, uuc_mc_index, bcast_mc_index, umc_mc_index,
        ipv4_unicast_enabled, ipv6_unicast_enabled,
        ipv4_multicast_mode, ipv6_multicast_mode,
        igmp_snooping_enabled, mld_snooping_enabled,
        ipv4_urpf_mode, ipv6_urpf_mode, exclusion_id) {
    modify_field(ingress_metadata.vrf, vrf);
    modify_field(ingress_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ingress_metadata.ipv6_unicast_enabled, ipv6_unicast_enabled);
    modify_field(ingress_metadata.ipv4_multicast_mode, ipv4_multicast_mode);
    modify_field(ingress_metadata.ipv6_multicast_mode, ipv6_multicast_mode);
    modify_field(ingress_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
    modify_field(ingress_metadata.mld_snooping_enabled, mld_snooping_enabled);
    modify_field(ingress_metadata.ipv4_urpf_mode, ipv4_urpf_mode);
    modify_field(ingress_metadata.ipv6_urpf_mode, ipv6_urpf_mode);
    modify_field(ingress_metadata.rmac_group, rmac_group);
    modify_field(ingress_metadata.bd_mrpf_group, mrpf_group);
    modify_field(ingress_metadata.uuc_mc_index, uuc_mc_index);
    modify_field(ingress_metadata.umc_mc_index, umc_mc_index);
    modify_field(ingress_metadata.bcast_mc_index, bcast_mc_index);
    modify_field(ingress_metadata.bd_label, bd_label);
    modify_field(intrinsic_metadata.exclusion_id1, exclusion_id);
}

table bd {
    reads {
        ingress_metadata.bd : exact;
    }
    actions {
        set_bd_info;
    }
    size : 16384;
}

control ingress {
    apply(bd);
}
