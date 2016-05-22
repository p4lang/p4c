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

header_type mac_da_t {
    fields {
        mac   : 48;
    }
}

header_type mac_sa_t {
    fields {
        mac   : 48;
    }
}

header_type len_or_type_t {
    fields {
        value   : 16;
    }
}

header_type pcp_t {
    fields {
        pcp : 3; 
    }
}

header_type cfi_t {
    fields {
        cfi : 1; 
    }
}

header_type vlan_id_t {
    fields {
        vlan_id_t : 12; 
    }
}


header mac_da_t      mac_da;
header mac_sa_t      mac_sa;
header len_or_type_t len_or_type;
header pcp_t         pcp;
header cfi_t         cfi;
header vlan_id_t     vlan_id;

parser start {
    return parse_mac_da;
}

parser parse_mac_da {
    extract(mac_da);
    return parse_mac_sa;
}

parser parse_mac_sa {
    extract (mac_sa);
    return parse_len_or_type;
}

parser parse_len_or_type {
    extract (len_or_type);
    return parse_pcp;
}

parser parse_pcp {
    extract(pcp);
    return parse_cfi;
}

parser parse_cfi {
    extract(cfi);
    return parse_vlan_id;
}

parser parse_vlan_id {
    extract(vlan_id);
    return ingress;
}

action nop() {
}

table t1 {
    reads {
        mac_da.mac : exact;
        len_or_type.value: exact;
    }
    actions {
        nop;
    }
}

table t2 {
    reads {
        mac_sa.mac : exact;
    }
    actions {
        nop;
    }
}

control ingress {
    apply(t1);
}

control egress {
    apply(t2);
}
