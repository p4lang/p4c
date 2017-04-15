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

#include <core.p4>

header EthernetHeader { bit<16> etherType; }
header IPv4           { bit<16> protocol; }
struct Packet_header {
    EthernetHeader ethernet;
    IPv4           ipv4;
}

parser EthernetParser(packet_in b,
                      out EthernetHeader h)
{ state start { transition accept; } }

parser GenericParser(packet_in b,
                     out Packet_header p)(bool udpSupport)
{
    EthernetParser() ethParser;

    state start {
        ethParser.apply(b, p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x0800 : ipv4;
        }
    }
    state ipv4 {
        b.extract(p.ipv4);
        transition select(p.ipv4.protocol) {
           16w6  : tryudp;
           16w17 : tcp;
        }
    }
    state tryudp {
        transition select(udpSupport) {
            false : reject;
            true  : udp;
        }
    }
    state udp {
        transition accept;
    }
    state tcp {
        transition accept;
    }
}
