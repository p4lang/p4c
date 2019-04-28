header ipv4_option_timestamp_t_1 {
    bit<8> value;
    bit<8> len;
}

#include <core.p4>
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<4> mcast_grp;
    bit<4> egress_rid;
}

struct my_metadata_t {
    bit<8> parse_ipv4_counter;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_base_t {
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

header ipv4_option_security_t {
    bit<8>  value;
    bit<8>  len;
    bit<72> security;
}

header ipv4_option_timestamp_t {
    bit<8>      value;
    bit<8>      len;
    @length(((bit<32>)len << 3) + 32w4294967280) 
    varbit<304> data;
}

header ipv4_option_EOL_t {
    bit<8> value;
}

header ipv4_option_NOP_t {
    bit<8> value;
}

struct metadata {
    bit<8> _my_metadata_parse_ipv4_counter0;
}

struct headers {
    @name(".ethernet") 
    ethernet_t              ethernet;
    @name(".ipv4_base") 
    ipv4_base_t             ipv4_base;
    @name(".ipv4_option_security") 
    ipv4_option_security_t  ipv4_option_security;
    @name(".ipv4_option_timestamp") 
    ipv4_option_timestamp_t ipv4_option_timestamp;
    @name(".ipv4_option_EOL") 
    ipv4_option_EOL_t[3]    ipv4_option_EOL;
    @name(".ipv4_option_NOP") 
    ipv4_option_NOP_t[3]    ipv4_option_NOP;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    ipv4_option_timestamp_t_1 tmp_hdr_0;
    ipv4_option_timestamp_t_1 tmp;
    bit<8> tmp_0;
    bit<16> tmp_1;
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_base_t>(hdr.ipv4_base);
        meta._my_metadata_parse_ipv4_counter0 = (bit<8>)((hdr.ipv4_base.ihl << 2) + 4w12);
        transition select(hdr.ipv4_base.ihl) {
            4w0x5: accept;
            default: parse_ipv4_options;
        }
    }
    @name(".parse_ipv4_option_EOL") state parse_ipv4_option_EOL {
        packet.extract<ipv4_option_EOL_t>(hdr.ipv4_option_EOL.next);
        meta._my_metadata_parse_ipv4_counter0 = meta._my_metadata_parse_ipv4_counter0 + 8w255;
        transition parse_ipv4_options;
    }
    @name(".parse_ipv4_option_NOP") state parse_ipv4_option_NOP {
        packet.extract<ipv4_option_EOL_t>(hdr.ipv4_option_NOP.next);
        meta._my_metadata_parse_ipv4_counter0 = meta._my_metadata_parse_ipv4_counter0 + 8w255;
        transition parse_ipv4_options;
    }
    @name(".parse_ipv4_option_security") state parse_ipv4_option_security {
        packet.extract<ipv4_option_security_t>(hdr.ipv4_option_security);
        meta._my_metadata_parse_ipv4_counter0 = meta._my_metadata_parse_ipv4_counter0 + 8w245;
        transition parse_ipv4_options;
    }
    @name(".parse_ipv4_option_timestamp") state parse_ipv4_option_timestamp {
        tmp_1 = packet.lookahead<bit<16>>();
        tmp.setValid();
        tmp.value = tmp_1[15:8];
        tmp.len = tmp_1[7:0];
        tmp_hdr_0 = tmp;
        packet.extract<ipv4_option_timestamp_t>(hdr.ipv4_option_timestamp, ((bit<32>)tmp_hdr_0.len << 3) + 32w4294967280);
        meta._my_metadata_parse_ipv4_counter0 = meta._my_metadata_parse_ipv4_counter0 - hdr.ipv4_option_timestamp.len;
        transition parse_ipv4_options;
    }
    @name(".parse_ipv4_options") state parse_ipv4_options {
        tmp_0 = packet.lookahead<bit<8>>();
        transition select(meta._my_metadata_parse_ipv4_counter0, tmp_0[7:0]) {
            (8w0x0 &&& 8w0xff, 8w0x0 &&& 8w0x0): accept;
            (8w0x0 &&& 8w0x0, 8w0x0 &&& 8w0xff): parse_ipv4_option_EOL;
            (8w0x0 &&& 8w0x0, 8w0x1 &&& 8w0xff): parse_ipv4_option_NOP;
            (8w0x0 &&& 8w0x0, 8w0x82 &&& 8w0xff): parse_ipv4_option_security;
            (8w0x0 &&& 8w0x0, 8w0x44 &&& 8w0xff): parse_ipv4_option_timestamp;
            default: noMatch;
        }
    }
    @header_ordering("ethernet", "ipv4_base", "ipv4_option_security", "ipv4_option_NOP", "ipv4_option_timestamp", "ipv4_option_EOL") @name(".start") state start {
        transition parse_ethernet;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".format_options_security") action format_options_security() {
        hdr.ipv4_option_NOP.pop_front(3);
        hdr.ipv4_option_EOL.pop_front(3);
        hdr.ipv4_option_EOL.push_front(1);
        hdr.ipv4_option_EOL[0].setValid();
        hdr.ipv4_base.ihl = 4w8;
    }
    @name(".format_options_timestamp") action format_options_timestamp() {
        hdr.ipv4_option_NOP.pop_front(3);
        hdr.ipv4_option_EOL.pop_front(3);
        hdr.ipv4_base.ihl = (bit<4>)(8w5 + (hdr.ipv4_option_timestamp.len >> 3));
    }
    @name(".format_options_both") action format_options_both() {
        hdr.ipv4_option_NOP.pop_front(3);
        hdr.ipv4_option_EOL.pop_front(3);
        hdr.ipv4_option_NOP.push_front(1);
        hdr.ipv4_option_NOP[0].setValid();
        hdr.ipv4_option_NOP[0].value = 8w0x1;
        hdr.ipv4_base.ihl = (bit<4>)(8w8 + (hdr.ipv4_option_timestamp.len >> 2));
    }
    @name("._nop") action _nop() {
    }
    @name(".format_options") table format_options_0 {
        actions = {
            format_options_security();
            format_options_timestamp();
            format_options_both();
            _nop();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.ipv4_option_security.isValid() : exact @name("ipv4_option_security.$valid$") ;
            hdr.ipv4_option_timestamp.isValid(): exact @name("ipv4_option_timestamp.$valid$") ;
        }
        size = 4;
        default_action = NoAction_0();
    }
    apply {
        format_options_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_base_t>(hdr.ipv4_base);
        packet.emit<ipv4_option_EOL_t>(hdr.ipv4_option_EOL[0]);
        packet.emit<ipv4_option_EOL_t>(hdr.ipv4_option_EOL[1]);
        packet.emit<ipv4_option_EOL_t>(hdr.ipv4_option_EOL[2]);
        packet.emit<ipv4_option_EOL_t>(hdr.ipv4_option_NOP[0]);
        packet.emit<ipv4_option_EOL_t>(hdr.ipv4_option_NOP[1]);
        packet.emit<ipv4_option_EOL_t>(hdr.ipv4_option_NOP[2]);
        packet.emit<ipv4_option_security_t>(hdr.ipv4_option_security);
        packet.emit<ipv4_option_timestamp_t>(hdr.ipv4_option_timestamp);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

struct tuple_0 {
    bit<4>                  field;
    bit<4>                  field_0;
    bit<8>                  field_1;
    bit<16>                 field_2;
    bit<16>                 field_3;
    bit<3>                  field_4;
    bit<13>                 field_5;
    bit<8>                  field_6;
    bit<8>                  field_7;
    bit<32>                 field_8;
    bit<32>                 field_9;
    ipv4_option_security_t  field_10;
    ipv4_option_EOL_t       field_11;
    ipv4_option_timestamp_t field_12;
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(true, { hdr.ipv4_base.version, hdr.ipv4_base.ihl, hdr.ipv4_base.diffserv, hdr.ipv4_base.totalLen, hdr.ipv4_base.identification, hdr.ipv4_base.flags, hdr.ipv4_base.fragOffset, hdr.ipv4_base.ttl, hdr.ipv4_base.protocol, hdr.ipv4_base.srcAddr, hdr.ipv4_base.dstAddr, hdr.ipv4_option_security, hdr.ipv4_option_NOP[0], hdr.ipv4_option_timestamp }, hdr.ipv4_base.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

