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
        // bool
        bool1 = (bool) hdr.ethernet.dstAddr[0:0];
        // TBD: Currently gives no error/warning from p4c, but
        // simple_switch gives error when trying to read the BMv2 JSON
        // file.
        log_msg("bool1={}", {bool1});

        // Casting a bool to a bit<1> and logging that bit<1> value
        // works fine, printing 1 for true, 0 for false.
        log_msg("(bit<1>) bool1={}", {(bit<1>) bool1});
        log_msg("(bit<1>) (!bool1)={}", {(bit<1>) (!bool1)});

        // bit<1>
        bit1 = hdr.ethernet.dstAddr[0:0];
        log_msg("bit1={}", {bit1});

        // signed int<W>
        signed1 = 127;
        signed1 = signed1 + 1;
        // Should wrap around to -128 in 2's complement, for 8-bit value.
        // Ideally should print that way, too, and it does in my
        // testing with simple_switch.
        log_msg("signed1={}", {signed1});

        // unsigned int<W>
        unsigned1 = 127;
        unsigned1 = unsigned1 + 1;
        // Should be unsigned 128, for unsigned int<8>, and it prints
        // that way in my simple_switch testing.
        log_msg("unsigned1={}", {unsigned1});

        // infinite precision int, compile-time constant value
        // p4c gives error "could not infer width"
//        log_msg("infint1={}", {5});

        // enum
        if (hdr.ethernet.dstAddr[1:0] == 0) {
            enum1 = MyEnum_t.VAL1;
        } else if (hdr.ethernet.dstAddr[1:0] == 1) {
            enum1 = MyEnum_t.VAL2;
        } else {
            enum1 = MyEnum_t.VAL3;
        }
        // In my testing, this prints the numeric code that p4c
        // selected for the MyEnum_t values at compile time, which can
        // change from run to run, although tends to be stable, I
        // think.  I would recommend against anyone _relying_ on it
        // being stable.
        log_msg("enum1={}", {enum1});

        // serializable enum
        if (hdr.ethernet.dstAddr[1:0] == 0) {
            serenum1 = MySerializableEnum_t.VAL17;
        } else if (hdr.ethernet.dstAddr[1:0] == 1) {
            serenum1 = MySerializableEnum_t.VAL23;
        } else {
            serenum1 = MySerializableEnum_t.VAL19;
        }
        // Prints the value of the underlying bit<10> value, as it
        // should.
        log_msg("serenum1={}", {serenum1});

        // header

        // TBD: Currently gives no error/warning from p4c, but
        // simple_switch gives the run-time error below when
        // attempting to execute this statement:

        // Assertion 'Default switch case should not be reachable' failed, file '../../include/bm/bm_sim/actions.h' line '334'.
        // Aborted

        // If simple_switch does not support this, and if it does not
        // seem worth the effort to enhance it for this, it seems
        // better if p4c would give an error that this is not
        // supported.

        // log_msg("hdr.ethernet={}", {hdr.ethernet});

        // struct - as of this writing, v1model.p4 standard_metadata_t
        // struct is defined such that all fields are of type bit<W>
        // for varying widths W.

        // TBD: Currently gives no error/warning from p4c, but
        // simple_switch gives the run-time error below when
        // attempting to execute this statement:

        // Assertion 'Default switch case should not be reachable' failed, file '../../include/bm/bm_sim/actions.h' line '334'.
        // Aborted

        // Same comments for this as for header types above.

        // log_msg("stdmeta={}", {stdmeta});

        // error

        // No error/warning from p4c, and simple_switch prints out the
        // numeric value that is assigned to the error code by p4c and
        // stored in the BMv2 JSON file, which can change over
        // different compilation runs of the same program, perhaps.
        // Not sure.  I am pretty sure it has changed across different
        // versions of p4c source code, occasionally.
        //log_msg("error.PacketTooShort={}", {error.PacketTooShort});
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
