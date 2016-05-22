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
        value   : 48;
    }
}

header mac_da_t      mac_da;
header mac_sa_t      mac_sa;
header len_or_type_t len_or_type;

parser start {
    extract(mac_da);
    extract(mac_sa);
    extract(len_or_type);
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
