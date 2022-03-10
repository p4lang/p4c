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
#include <v1model.p4>
/**
 * Table with a match kind M, a key type K, and a value type V.  It
 * has only a lookup method from the data plane.  Modifying it
 * requires control plane software API calls.
 *
 * "Value" means that the lookup operation always returns a value of
 * type V, without also returning a hit/miss indication.  The
 * implementation of a ValueLookupTable must always have either (a) a
 * default value to return if the lookup of the key experiences a
 * miss, or (b) a guarantee that the set of keys always hits.
 *
 * It would be most convenient for control plane API writers if the
 * control plane API was the same as a P4 table defined as follows.
 * Note that I have proposed names for the fields, the action, and the
 * action parameter, which do not exist anywhere in the extern
 * definition below, because the extern definition is not a P4 table
 * definition.  We could consider defining some way to specify the
 * action and parameter name.  The key field names being the same as
 * the member names of the struct type K seems like a reasonable
 * choice, and I doubt someone would have a strong desire to customize
 * those, other than by changing the names of the member fields in
 * type K.

// TODO: The handling of match_kind tuples below and the type M1 is
// probably wrong.  I have not written P4 code like that before, so I
// am unfamiliar with it.  What is written below is my best first
// guess for those parts.

struct K1 {
    T1 f1;
    T2 f2;
    // ... can be any number of fields here ...
}

// The number of elements of the tuple type M1 must be the same as the
// number of fields in struct K1 above.
tuple<mk1, mk2, ...> M1;

// This instantiation:
ValueLookupTable<M1, K1, V1>({mk1, mk2, ...}, my_size1, default_value1) vlt1;

// has the same control plane API as this P4 table:
action vlt1_set_value (V1 value) {
    // no body needed here to define the control plane API of a table
}
table vlt1 {
    key = {
        f1: mk1;
        f2: mk2;
        // ... etc.
    }
    actions = {
        vlt1_set_value;
    }
    const default_action = vlt1_set_value(default_value1);
    size = my_size1;
}

 *
 * @param K  must be a struct type
 * @param M  each field of the struct M must be of type match_kind.
 *           should have the same number of fields as K.
 */
extern ValueLookupTable<M, K, V> {
    /**
     * Create a table with match kinds M and the specified size
     * (number of entries).  The default value returned when a lookup
     * experiences a miss is given by default_value.
     */
    ValueLookupTable(M match_kinds_tuple, int size, V default_value);

    /**
     * The same as the constructor with an explicit default_value,
     * except the default_value is the default value for the type V as
     * defined in the section "Default values" of the P4_16 language
     * specification.
     */
    ValueLookupTable(M match_kinds_tuple, int size);

    /**
     * Look up the key in the table.  Always hits, so always returns a
     * value of type V.
     */
    V lookup(in K key);
}

/**
 * Same as ValueLookupTable, except there is no type M parameter,
 * and the match_kind of all lookup key fields is restricted to be
 * exact.
 */
extern ValueExactLookupTable<K, V> {
    /**
     * Create a table with match kinds all 'exact' and the specified
     * size (number of entries).  The default value returned when a
     * lookup experiences a miss is given by default_value.
     */
    ValueExactLookupTable(int size, V default_value);

    /**
     * The same as the constructor with an explicit default_value,
     * except the default_value is the default value for the type V as
     * defined in the section "Default values" of the P4_16 language
     * specification.
     */
    ValueExactLookupTable(int size);

    /**
     * Look up the key in the table.  Always hits, so always returns a
     * value of type V.
     */
    V lookup(in K key);
}


/**
 * Type ValueOrMiss<T> is not as "clean" in the sense of returning
 * fully specified values, and perhaps also less clean in some type
 * safety sense, as Mihai's proposal of a type Option<T> as shown
 * here: https://github.com/p4lang/p4c/pull/2739/files
 *
 * The only reason I am proposing it here is that it is already
 * implemented in today's p4c, whereas the 'union' keyword -- used in
 * Mihai's example linked to above -- is not.
 */
struct ValueOrMiss<T> {
    bool hit;
    T    value;
}

/**
 * Table with a match kind M, a key type K, and a value type V.  It
 * has only a lookup method from the data plane.  Modifying the set of
 * key/value pairs requires control plane software API calls.
 *
 * @param K  must be a struct type
 * @param M  each field of the struct M must be of type match_kind.
 *           should have the same number of fields as K.
 */
extern LookupTable<M, K, V> {
    /**
     * Create a table with match kinds M and the specified size
     * (number of entries).
     */
    LookupTable(M match_kinds_tuple, int size);

    /**
     * Look up the key in the table.
     *
     * The return value r is one of these two cases:
     *
     * + r.hit is true, and r.value is defined by the control plane
     *   API call that added or last modified the entry.
     *
     * + r.hit is false, and r.value is an unspecified value of type
     *   V.  This value should not be relied upon to have a predictable
     *   value.
     */
    ValueOrMiss<V> lookup(in K key);
}

extern bit<8> fn_foo<T>(in T data);

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

parser parserImpl(
    packet_in pkt,
    out headers_t hdr,
    inout metadata_t meta,
    inout standard_metadata_t stdmeta)
{
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(
    inout headers_t hdr,
    inout metadata_t meta)
{
    apply { }
}

typedef tuple <match_kind> mk1_t;

const tuple<match_kind> exact_once = {exact};

control ingressImpl(
    inout headers_t hdr,
    inout metadata_t meta,
    inout standard_metadata_t stdmeta)
{
    ValueLookupTable<tuple<match_kind>, bit<8>, bit<16>>(exact_once, 1024) lut3;

    apply {
        if (hdr.ethernet.isValid()) {
            hdr.ethernet.dstAddr[47:40] = 5;
            hdr.ethernet.etherType[7:0] = fn_foo(ternary);

            hdr.ethernet.dstAddr[15:0] = lut3.lookup(hdr.ethernet.etherType[7:0]);
        }
    }
}

control egressImpl(
    inout headers_t hdr,
    inout metadata_t meta,
    inout standard_metadata_t stdmeta)
{
    apply { }
}

control updateChecksum(
    inout headers_t hdr,
    inout metadata_t meta)
{
    apply { }
}

control deparserImpl(
    packet_out pkt,
    in headers_t hdr)
{
    apply {
        pkt.emit(hdr.ethernet);
    }
}

V1Switch(parserImpl(),
         verifyChecksum(),
         ingressImpl(),
         egressImpl(),
         updateChecksum(),
         deparserImpl()) main;
