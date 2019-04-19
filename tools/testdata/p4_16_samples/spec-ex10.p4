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

struct Ethernet_h { }
struct Ip_h { }
header Tcp_h { }
header Udp_h { } 
header_union L4_h {
   Tcp_h tcp;
   Udp_h udp;
}
struct Parsed_headers
{
    Ethernet_h ethernet;
    Ip_h       ip;
    L4_h       tcpudp;
}
