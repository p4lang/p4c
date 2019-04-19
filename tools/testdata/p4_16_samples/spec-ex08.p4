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

header Empty_h {}

header Ethernet_h {
   bit<48> dstAddr;
   bit<48> srcAddr;
   bit<16> etherType;
}

control p()
{
    apply {
        Ethernet_h ethernetHeader;
    }
}

header Ipv4_h {
   bit<4>       version;
   bit<4>       ihl;
   bit<8>       diffserv;
   bit<16>      totalLen;
   bit<16>      identification;
   bit<3>       flags;
   bit<13>      fragOffset;
   bit<8>       ttl;
   bit<8>       protocol;
   bit<16>      hdrChecksum;
   bit<32>      srcAddr;
   bit<32>      dstAddr;
   varbit<160>  options;
}

header Ipv6_h { }
header_union Ip_h { 
   Ipv4_h ipv4;
   Ipv6_h ipv6;
}
