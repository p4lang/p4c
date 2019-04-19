header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 3;
        ecn : 3;
        ctrl : 6;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

parser start {
    return parse_ethernet;
}

#define ETHERTYPE_IPV4 0x0800

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        default: ingress;
    }
}

header ipv4_t ipv4;

#define IP_PROTOCOLS_TCP 6

parser parse_ipv4 {
    extract(ipv4);
    return select(latest.protocol) {
        IP_PROTOCOLS_TCP : parse_tcp;
        default: ingress;
    }
}

header tcp_t tcp;

parser parse_tcp {
    extract(tcp);
    return ingress;
}

// TODO: Define the threshold value
#define HEAVY_HITTER_THRESHOLD 100

field_list ipv4_checksum_list {
        ipv4.version;
        ipv4.ihl;
        ipv4.diffserv;
        ipv4.totalLen;
        ipv4.identification;
        ipv4.flags;
        ipv4.fragOffset;
        ipv4.ttl;
        ipv4.protocol;
        ipv4.srcAddr;
        ipv4.dstAddr;
}

field_list_calculation ipv4_checksum {
    input {
        ipv4_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4.hdrChecksum  {
    verify ipv4_checksum;
    update ipv4_checksum;
}

action _drop() {
    drop();
}

header_type custom_metadata_t {
    fields {
        nhop_ipv4: 32;
        // TODO: Add the metadata for hash indices and count values
        hash_val1: 16;
        hash_val2: 16;
        count_val1: 16;
        count_val2: 16;
    }
}

metadata custom_metadata_t custom_metadata;

action set_nhop(nhop_ipv4, port) {
    modify_field(custom_metadata.nhop_ipv4, nhop_ipv4);
    modify_field(standard_metadata.egress_spec, port);
    add_to_field(ipv4.ttl, -1);
}

action set_dmac(dmac) {
    modify_field(ethernet.dstAddr, dmac);
}

// TODO: Define the field list to compute the hash on
// Use the 5 tuple of
// (src ip, dst ip, src port, dst port, ip protocol)

field_list hash_fields {
    ipv4.srcAddr;
    ipv4.dstAddr;
    ipv4.protocol;
    tcp.srcPort;
    tcp.dstPort;
}

// TODO: Define two different hash functions to store the counts
// Please use csum16 and crc16 for the hash functions
field_list_calculation heavy_hitter_hash1 {
    input {
        hash_fields;
    }
    algorithm : csum16;
    output_width : 16;
}

field_list_calculation heavy_hitter_hash2 {
    input {
        hash_fields;
    }
    algorithm : crc16;
    output_width : 16;
}

// TODO: Define the registers to store the counts
register heavy_hitter_counter1{
    width : 16;
    instance_count : 16;
}

register heavy_hitter_counter2{
    width : 16;
    instance_count : 16;
}

// TODO: Actions to set heavy hitter filter
action set_heavy_hitter_count() {
    modify_field_with_hash_based_offset(custom_metadata.hash_val1, 0,
                                        heavy_hitter_hash1, 16);
    register_read(custom_metadata.count_val1, heavy_hitter_counter1, custom_metadata.hash_val1);
    add_to_field(custom_metadata.count_val1, 1);
    register_write(heavy_hitter_counter1, custom_metadata.hash_val1, custom_metadata.count_val1);

    modify_field_with_hash_based_offset(custom_metadata.hash_val2, 0,
                                        heavy_hitter_hash2, 16);
    register_read(custom_metadata.count_val2, heavy_hitter_counter2, custom_metadata.hash_val2);
    add_to_field(custom_metadata.count_val2, 1);
    register_write(heavy_hitter_counter2, custom_metadata.hash_val2, custom_metadata.count_val2);
}

// TODO: Define the tables to run actions
table set_heavy_hitter_count_table {
    actions {
        set_heavy_hitter_count;
    }
    size: 1;
}

// TODO: Define table to drop the heavy hitter traffic
table drop_heavy_hitter_table {
    actions { _drop; }
    size: 1;
}

table ipv4_lpm {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
        set_nhop;
        _drop;
    }
    size: 1024;
}

table forward {
    reads {
        custom_metadata.nhop_ipv4 : exact;
    }
    actions {
        set_dmac;
        _drop;
    }
    size: 512;
}

action rewrite_mac(smac) {
    modify_field(ethernet.srcAddr, smac);
}

table send_frame {
    reads {
        standard_metadata.egress_port: exact;
    }
    actions {
        rewrite_mac;
        _drop;
    }
    size: 256;
}

control ingress {
    // TODO: Add table control here
    apply(set_heavy_hitter_count_table);
    if (custom_metadata.count_val1 > HEAVY_HITTER_THRESHOLD and custom_metadata.count_val2 > HEAVY_HITTER_THRESHOLD) {
        apply(drop_heavy_hitter_table);
    } else {
        apply(ipv4_lpm);
        apply(forward);
    }
}

control egress {
    apply(send_frame);
}
