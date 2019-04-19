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

#include <ebpf_model.p4>
#include <core.p4>

#include "ebpf_headers.p4"

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

parser prs(packet_in p, out Headers_t headers) {
    state start  {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800 : ip;
            default : reject;
        }
    }

    state ip {
        p.extract(headers.ipv4);
        transition accept;
    }
}

control Check(in IPv4Address address, inout bool pass) {
    action Reject() {
        pass = false;
    }

    table Check_ip {
        key = { address : exact; }
        actions = {
            Reject;
            NoAction;
        }

        implementation = hash_table(1024);
        const default_action = NoAction;
    }

    apply {
        Check_ip.apply();
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    Check() c1;
    apply {
        pass = true;

        if (!headers.ipv4.isValid()) {
            pass = false;
            return;
        }

        c1.apply(headers.ipv4.srcAddr, pass);
        c1.apply(headers.ipv4.dstAddr, pass);
    }
}

ebpfFilter(prs(), pipe()) main;
