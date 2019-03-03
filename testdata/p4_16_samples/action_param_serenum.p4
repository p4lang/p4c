/*
Copyright 2016 VMware, Inc.

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
#include <core.p4>

enum bit<16> EthTypes {
    IPv4 = 0x0800,
    ARP = 0x0806,
    RARP = 0x8035,
    EtherTalk = 0x809B,
    VLAN = 0x8100,
    IPX = 0x8137,
    IPv6 = 0x86DD
}

struct standard_metadata_t {
    EthTypes instance_type;
}    
control c(inout standard_metadata_t sm, in EthTypes eth) {
    action a(in EthTypes type) { sm.instance_type = type; }
    table t {
	actions = { a(eth); }
	default_action = a(eth);
    }

    apply {
        t.apply();
    }
}

control proto(inout standard_metadata_t sm, in EthTypes eth);
package top(proto p);

top(c()) main;
