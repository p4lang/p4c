/*
Copyright 2019 Cisco Systems, Inc.

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
#include <v1model.p4>

extern Random2 {
  Random2();
  bit<10> read();
}

typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct headers_t {
    ethernet_t    ethernet;
}

struct metadata_t {
}

parser parserImpl(packet_in packet,
                  out headers_t hdr,
                  inout metadata_t meta,
                  inout standard_metadata_t stdmeta)
{
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply { }
}

control foo2(inout headers_t my_headers,
             inout metadata_t meta,
             register<bit<8>> my_reg)
{
    bit<32> idx;
    bit<8> val;

    action foo2_action() {
        // With the two lines below mentioning 'my_reg' in the
        // program, the latest p4test as of 2019-May-23 gives an
        // error (and p4c-bm2-ss gives same error):

        // error: my_reg: Not found declaration

        // With those lines removed, that version of p4test gives no
        // error, and p4c-bm2-ss can compile the program to a BMv2
        // JSON file.
        idx = (bit<32>) my_headers.ethernet.srcAddr[7:0];
        my_reg.read(val, idx);
        val = val - 7;
        my_reg.write(idx, val);
    }
    table foo2_table {
        key = {
            my_headers.ethernet.srcAddr : exact;
        }
        actions = {
            foo2_action;
            NoAction;
        }
        default_action = NoAction;
    }
    apply {
        foo2_table.apply();
    }
}

control ingressImpl(inout headers_t hdr,
                    inout metadata_t meta,
                    inout standard_metadata_t stdmeta)
{
    register<bit<8>>(256) reg1;
    foo2() foo2_inst;
    apply {
        stdmeta.egress_spec = 0;
        foo2_inst.apply(hdr, meta, reg1);
    }
}

control egressImpl(inout headers_t hdr,
                   inout metadata_t meta,
                   inout standard_metadata_t stdmeta)
{
    apply { }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply { }
}

control deparserImpl(packet_out packet,
                     in headers_t hdr)
{
    apply {
        packet.emit(hdr.ethernet);
    }
}

V1Switch(parserImpl(),
         verifyChecksum(),
         ingressImpl(),
         egressImpl(),
         updateChecksum(),
         deparserImpl()) main;
