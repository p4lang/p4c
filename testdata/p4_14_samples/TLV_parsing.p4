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

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type ipv4_base_t {
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

// End of Option List
#define IPV4_OPTION_EOL_VALUE 0x00
header_type ipv4_option_EOL_t {
    fields {
        value : 8;
    }
}

// No operation
#define IPV4_OPTION_NOP_VALUE 0x01
header_type ipv4_option_NOP_t {
    fields {
        value : 8;
    }
}

#define IPV4_OPTION_SECURITY_VALUE 0x82
header_type ipv4_option_security_t {
    fields {
        value : 8;
        len : 8;
        security : 72;
    }
}

#define IPV4_OPTION_TIMESTAMP_VALUE 0x44
header_type ipv4_option_timestamp_t {
    fields {
        value : 8;
        len : 8;
        data : *;
    }
    length : len;
    max_length : 40;
}

header_type intrinsic_metadata_t {
    fields {
        mcast_grp : 4;
        egress_rid : 4;
        mcast_hash : 16;
        lf_field_list: 32;
    }
}

header_type my_metadata_t {
    fields {
        parse_ipv4_counter : 8;
    }
}

header ethernet_t ethernet;
header ipv4_base_t ipv4_base;
header ipv4_option_EOL_t ipv4_option_EOL[3];
header ipv4_option_NOP_t ipv4_option_NOP[3];
header ipv4_option_security_t ipv4_option_security;
header ipv4_option_timestamp_t ipv4_option_timestamp;
metadata intrinsic_metadata_t intrinsic_metadata;
metadata my_metadata_t my_metadata;

@pragma header_ordering ethernet ipv4_base ipv4_option_security ipv4_option_NOP ipv4_option_timestamp ipv4_option_EOL
parser start {
    return parse_ethernet;
}

#define ETHERTYPE_IPV4 0x0800
parser parse_ethernet {
    extract(ethernet);
    return select(ethernet.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        default: ingress;   
    }
}

parser parse_ipv4 {
    extract(ipv4_base);
    set_metadata(my_metadata.parse_ipv4_counter, ipv4_base.ihl * 4 - 20);
    return select(ipv4_base.ihl) {
        0x05 : ingress;
        default : parse_ipv4_options;
    }
}

parser parse_ipv4_options {
    // match on byte counter and option value
    return select(my_metadata.parse_ipv4_counter, current(0, 8)) {
        0x0000 mask 0xff00 : ingress;
        0x0000 mask 0x00ff : parse_ipv4_option_EOL;
        0x0001 mask 0x00ff : parse_ipv4_option_NOP;
        0x0082 mask 0x00ff : parse_ipv4_option_security;
        0x0044 mask 0x00ff : parse_ipv4_option_timestamp;
    }
}

parser parse_ipv4_option_EOL {
    extract(ipv4_option_EOL[next]);
    set_metadata(my_metadata.parse_ipv4_counter,
                 my_metadata.parse_ipv4_counter - 1);
    return parse_ipv4_options;
}

parser parse_ipv4_option_NOP {
    extract(ipv4_option_NOP[next]);
    set_metadata(my_metadata.parse_ipv4_counter,
                 my_metadata.parse_ipv4_counter - 1);
    return parse_ipv4_options;
}

parser parse_ipv4_option_security {
    extract(ipv4_option_security);
    // security option must have length 11 bytes
    set_metadata(my_metadata.parse_ipv4_counter,
                 my_metadata.parse_ipv4_counter - 11);
    return parse_ipv4_options;
}

parser parse_ipv4_option_timestamp {
    extract(ipv4_option_timestamp);
    set_metadata(my_metadata.parse_ipv4_counter,
                 my_metadata.parse_ipv4_counter - ipv4_option_timestamp.len);
    return parse_ipv4_options;
}

field_list ipv4_checksum_list {
        ipv4_base.version;
        ipv4_base.ihl;
        ipv4_base.diffserv;
        ipv4_base.totalLen;
        ipv4_base.identification;
        ipv4_base.flags;
        ipv4_base.fragOffset;
        ipv4_base.ttl;
        ipv4_base.protocol;
        ipv4_base.srcAddr;
        ipv4_base.dstAddr;
        ipv4_option_security;
        ipv4_option_NOP[0];
        ipv4_option_timestamp;        
}

field_list_calculation ipv4_checksum {
    input {
        ipv4_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4_base.hdrChecksum  {
    update ipv4_checksum;
}

action _drop() {
    drop();
}

action _nop() {
}

control ingress {

}

action format_options_security() {
    pop(ipv4_option_NOP, 3);
    pop(ipv4_option_EOL, 3);
    push(ipv4_option_EOL, 1);
    modify_field(ipv4_base.ihl, 8);
}

action format_options_timestamp() {
    pop(ipv4_option_NOP, 3);
    pop(ipv4_option_EOL, 3);
    // timestamp option is word-aligned so no need for NOP or EOL
    modify_field(ipv4_base.ihl, 5 + (ipv4_option_timestamp.len >> 3));
}

action format_options_both() {
    pop(ipv4_option_NOP, 3);
    pop(ipv4_option_EOL, 3);
    push(ipv4_option_NOP, 1);
    modify_field(ipv4_option_NOP[0].value, IPV4_OPTION_NOP_VALUE);
    modify_field(ipv4_base.ihl, 8 + (ipv4_option_timestamp.len >> 2));
}

table format_options {
    reads {
        ipv4_option_security : valid;
        ipv4_option_timestamp : valid;
    }
    actions {
        format_options_security;
        format_options_timestamp;
        format_options_both;
        _nop;
    }
    size : 4;
}

control egress {
    apply(format_options);
}
