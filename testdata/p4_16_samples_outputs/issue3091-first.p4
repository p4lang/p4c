#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

extern ValueLookupTable<M, K, V> {
    ValueLookupTable(M match_kinds_tuple, int size, V default_value);
    ValueLookupTable(M match_kinds_tuple, int size);
    V lookup(in K key);
}

extern ValueExactLookupTable<K, V> {
    ValueExactLookupTable(int size, V default_value);
    ValueExactLookupTable(int size);
    V lookup(in K key);
}

struct ValueOrMiss<T> {
    bool hit;
    T    value;
}

extern LookupTable<M, K, V> {
    LookupTable(M match_kinds_tuple, int size);
    ValueOrMiss<V> lookup(in K key);
}

extern bit<8> fn_foo<T>(in T data);
typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser parserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

typedef tuple<match_kind> mk1_t;
const tuple<match_kind> exact_once = { exact };
control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    ValueLookupTable<tuple<match_kind>, bit<8>, bit<16>>({ exact }, 1024) lut3;
    apply {
        if (hdr.ethernet.isValid()) {
            hdr.ethernet.dstAddr[47:40] = 8w5;
            hdr.ethernet.etherType[7:0] = fn_foo<match_kind>(ternary);
            hdr.ethernet.dstAddr[15:0] = lut3.lookup(hdr.ethernet.etherType[7:0]);
        }
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
