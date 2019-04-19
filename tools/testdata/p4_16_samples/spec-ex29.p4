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

header Ethernet { bit<16> etherType; }
header IPv4 {
   bit<4>       version;
   bit<4>      ihl;
   bit<8>       diffserv;
   bit<16>     totalLen;
   bit<16>      identification;
   bit<3>       flags;
   bit<13>      fragOffset;
   bit<8>      ttl;
   bit<8>       protocol;
   bit<16>      hdrChecksum;
   bit<32>      srcAddr;
   bit<32>      dstAddr;
}

header IPv6 {}
header_union IP {
    IPv4 ipv4;
    IPv6 ipv6;
}
struct Parsed_packet {
   Ethernet ethernet;
   IP ip;
}

error { IPv4FragmentsNotSupported, IPv4OptionsNotSupported, IPv4IncorrectVersion }

parser top(packet_in b, out Parsed_packet p) {
    state start {
       b.extract(p.ethernet);
       transition select(p.ethernet.etherType) {
           16w0x0800 : parse_ipv4;
           16w0x86DD : parse_ipv6;
       }
   }

   state parse_ipv4 {
       b.extract(p.ip.ipv4);
       verify(p.ip.ipv4.version == 4w4, error.IPv4IncorrectVersion);
       verify(p.ip.ipv4.ihl == 4w5, error.IPv4OptionsNotSupported);
       verify(p.ip.ipv4.fragOffset == 13w0, error.IPv4FragmentsNotSupported);
       transition accept;
   }

   state parse_ipv6 {
       b.extract(p.ip.ipv6);
       transition accept;
   }
}

control Automatic(packet_out b, in Parsed_packet p) {
    apply {
        b.emit(p.ethernet);
        b.emit(p.ip.ipv6);
        b.emit(p.ip.ipv4);
    }
}
