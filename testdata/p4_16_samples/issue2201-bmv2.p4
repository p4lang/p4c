/*
Copyright 2020 Cisco Systems, Inc.

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

typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct headers_t {
    ethernet_t    ethernet;
    ipv4_t        ipv4;
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
        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply { }
}

enum MyEnum_t {
    VAL1,
    VAL2,
    VAL3
}

enum bit<10> MySerializableEnum_t {
    VAL17 = 17,
    VAL23 = 23,
    VAL19 = 19
}

control ingressImpl(inout headers_t hdr,
                    inout metadata_t meta,
                    inout standard_metadata_t stdmeta)
{
    bool bool1;
    bit<1> bit1;
    MyEnum_t enum1;
    MySerializableEnum_t serenum1;
    int<8> signed1;
    bit<8> unsigned1;

    apply {
        // Comments below describe how each type is printed as of
        // p4c-bm2-ss and behavioral-model (aka BMv2) simple_switch as
        // of 2022-Oct latest code.

        log_msg("GREPME Packet ingress begin");
        
        // bool value prints as 1 for true, 0 for false
        bool1 = (bool) hdr.ethernet.dstAddr[0:0];
        log_msg("GREPME bool1={}", {bool1});

        // bool cast to bit<1> value prints as 1 for true, 0 for false
        log_msg("GREPME (bit<1>) bool1={}", {(bit<1>) bool1});
        log_msg("GREPME (bit<1>) (!bool1)={}", {(bit<1>) (!bool1)});

        // bit<1> prints as 0 or 1
        bit1 = hdr.ethernet.dstAddr[0:0];
        log_msg("GREPME bit1={}", {bit1});

        // signed int<W>
        signed1 = 127;
        signed1 = signed1 + 1;
        // Should wrap around to -128 in 2's complement, for 8-bit value.
        // Does print as -128 with simple_switch
        log_msg("GREPME signed1={}", {signed1});

        // unsigned int<W>
        unsigned1 = 127;
        unsigned1 = unsigned1 + 1;
        // Should be unsigned 128, for unsigned bit<8>
        // Prints as 128 with simple_switch
        log_msg("GREPME unsigned1={}", {unsigned1});

        // infinite precision int, compile-time constant value
        // p4c gives error "could not infer width"
        //log_msg("GREPME infint1={}", {5});

        // bit<2> from slice
        // Prints as unsigned decimal number
        log_msg("GREPME hdr.ethernet.dstAddr[1:0]={}", {hdr.ethernet.dstAddr[1:0]});

        // enum
        if (hdr.ethernet.dstAddr[1:0] == 0) {
            enum1 = MyEnum_t.VAL1;
        } else if (hdr.ethernet.dstAddr[1:0] == 1) {
            enum1 = MyEnum_t.VAL2;
        } else {
            enum1 = MyEnum_t.VAL3;
        }
        // In my testing, this prints the numeric code that p4c
        // selected for the MyEnum_t values at compile time, which has
        // is a BMv2-implementation-specific integer value.  I would
        // recommend against anyone _relying_ on it being stable.  For
        // an enum with 3 members, I believe this
        // BMv2-implementation-specific value will always be 0, 1, or
        // 2.
        log_msg("GREPME enum1={}", {enum1});

        // serializable enum
        if (hdr.ethernet.dstAddr[1:0] == 0) {
            serenum1 = MySerializableEnum_t.VAL17;
        } else if (hdr.ethernet.dstAddr[1:0] == 1) {
            serenum1 = MySerializableEnum_t.VAL23;
        } else {
            serenum1 = MySerializableEnum_t.VAL19;
        }
        // Prints the value of the underlying bit<10> value as
        // unsigned decimal number, as it should.
        log_msg("GREPME serenum1={}", {serenum1});

        // header

        // An ipv4 header prints as shown in this example.  All field
        // values are shown as unsigned decimal integers for this
        // program, since ipv4_t's fields are all of type bit<W> for
        // some bit width W.
        // (version:4,ihl:5,diffserv:0,totalLen:20,identification:1,flags:0,fragOffset:0,ttl:64,protocol:0,hdrChecksum:31975,srcAddr:2130706433,dstAddr:2130706433)

        // Unfortunately an invalid ipv4 header also prints the same
        // way in simple_switch, with all field values equal to 0.
        // However, that is not actually enough information to
        // determine from the printed representation whether
        // hdr.ipv4.isValid() is true or false.
        
        log_msg("GREPME hdr.ethernet.isValid()={}", {hdr.ethernet.isValid()});
        log_msg("GREPME hdr.ethernet={}", {hdr.ethernet});
        log_msg("GREPME hdr.ipv4.isValid()={}", {hdr.ipv4.isValid()});
        log_msg("GREPME hdr.ipv4={}", {hdr.ipv4});

        // struct - as of this writing, v1model.p4 standard_metadata_t
        // struct is defined such that all fields are of type bit<W>
        // for varying widths W.

        // The stdmeta struct prints as shown in this example by
        // simple_switch:
        // (ingress_port:1,egress_spec:0,egress_port:0,instance_type:0,packet_length:34,enq_timestamp:0,enq_qdepth:0,deq_timedelta:0,deq_qdepth:0,ingress_global_timestamp:26469967,egress_global_timestamp:0,mcast_grp:0,egress_rid:0,checksum_error:0,parser_error:0,priority:0)
        log_msg("GREPME stdmeta={}", {stdmeta});

        // error

        // simple_switch prints out the numeric value that is assigned
        // to the error code by p4c and stored in the BMv2 JSON file,
        // which is a BMv2-implementation-specific numeric value.
        log_msg("GREPME error.PacketTooShort={}", {error.PacketTooShort});
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
