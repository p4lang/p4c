/*
Copyright 2022 Intel Corporation

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
#include <pna.p4>

header my_header_t {
    bit<16> type1;
    bit<8>  type2;
    bit<32> value;
}

struct main_metadata_t {
}

struct headers_t {
    my_header_t h1;
    my_header_t h2;
}

parser MainParserImpl(
          packet_in                        pkt,
    out   headers_t                        hdr,
    inout main_metadata_t                  main_meta,
    in    pna_main_parser_input_metadata_t istd)
{
    my_header_t local_hdr;
    state start {
        pkt.extract(local_hdr);
        transition select(local_hdr.type1) {
            16w0x1234: parse_h1;
            16w0x5678: parse_h2;
            default: accept;
        }
    }

    state parse_h1 {
        pkt.extract(hdr.h1);
        transition accept;
    }

    state parse_h2 {
        //hdr.h2 = local_hdr;
        hdr.h2.setValid();
        hdr.h2.type1 = local_hdr.type1;
        hdr.h2.type2 = local_hdr.type2;
        hdr.h2.value = local_hdr.value;
        transition accept;
    }
}

control PreControlImpl(
    in    headers_t                 hdr,
    inout main_metadata_t           meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply {}
}

control MainControlImpl(
    inout headers_t                  hdr,
    inout main_metadata_t            user_meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd)
{
    apply {
    }
}

control MainDeparserImpl(
       packet_out                 pkt,
    in headers_t                  hdr,
    in main_metadata_t            user_meta,
    in pna_main_output_metadata_t ostd)
{
    apply {
        if (hdr.h1.isValid() && hdr.h2.isValid()) {
            pkt.emit(hdr.h1);
            pkt.emit(hdr.h2);
        }
    }
}

PNA_NIC(
    MainParserImpl(),
    PreControlImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    ) main;
