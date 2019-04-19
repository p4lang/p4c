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

// IPv4 header without options
header Ipv4_no_options_h {
    bit<4>   version;
    bit<4>  ihl;
    bit<8>   diffserv;
    bit<16> totalLen;
    bit<16>  identification;
    bit<3>   flags;
    bit<13>  fragOffset;
    bit<8>  ttl;
    bit<8>   protocol;
    bit<16>  hdrChecksum;
    bit<32>  srcAddr;
    bit<32>  dstAddr;
}

header Ipv4_options_h {
    varbit<160> options;
}

header Tcp {
    bit<16> port;
}

struct Parsed_headers {
    Ipv4_no_options_h ipv4;
    Ipv4_options_h    ipv4options;
    Tcp               tcp;
}

error { InvalidOptions }

parser Top(packet_in b, out Parsed_headers headers) {
    state start {
        transition parse_ipv4;
    }

    state parse_ipv4 {
        b.extract(headers.ipv4);
        verify(headers.ipv4.ihl >= 4w5, error.InvalidOptions);
        transition parse_ipv4_options;
    }

    state parse_ipv4_options
    {
        b.extract(headers.ipv4options,
            (bit<32>)(8w8 * ((bit<8>)headers.ipv4.ihl * 8w4 - 8w20)));
        transition select (headers.ipv4.protocol) {
            8w6     : parse_tcp;
            8w17    : parse_udp;
            default : accept;
        }
    }

    state parse_tcp {
        b.extract(headers.tcp);
        transition select (headers.tcp.port)
        {
            16w0 &&& 16w0xFC00 : well_known_port; // top 6 bits are zero
            default : other_port;
        }
    }

    state well_known_port { transition accept; }

    state other_port { transition accept; }

    state parse_udp { transition accept; }
}
