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

header IPv4_option_NOP {
    bit<8> value;
}

struct Parsed_Packet {
    IPv4_option_NOP[3] nop;
}

parser Parser(packet_in b, out Parsed_Packet p) {
    state start {
        transition select(8w0, b.lookahead<bit<8>>()) {
            default : accept;
            (0, 0 &&& 0) : accept;
            (0 &&& 0, 0x44) : ipv4_option_NOP;
        }
    }

    state ipv4_option_NOP {
        b.extract(p.nop.next);
        transition start;
    }

}

package Switch();

Switch() main;
