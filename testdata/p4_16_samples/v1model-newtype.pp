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

// This file has been renamed to use suffix pp because it is not a complete p4 file

/* P4-16 declaration of the P4 v1.0 switch model */

#ifndef _V1_MODEL_P4_
#define _V1_MODEL_P4_

#include "core.p4"

match_kind {
    range,
    // Used for implementing dynamic_action_selection
    selector
}

// Are these correct?
@metadata @name("standard_metadata")
struct standard_metadata_t {
    PortId_t ingress_port;
    PortId_t egress_spec;
    PortId_t egress_port;
    bit<32> clone_spec;
    bit<32> instance_type;
    // deleted deprecated drop and recirculate_port fields from this
    // slightly modified copy of v1model.p4, since this file is only
    // #include'd in one test program that makes no mention of them.
    //bit<1>  drop;
    //bit<16> recirculate_port;
    bit<32> packet_length;
    //
    // @alias is used to generate the field_alias section of the BMV2 JSON.
    // Field alias creates a mapping from the metadata name in P4 program to
    // the behavioral model's internal metadata name. Here we use it to
    // expose all metadata supported by simple switch to the user through
    // standard_metadata_t.
    //
    // flattening fields that exist in bmv2-ss
    // queueing metadata
    @alias("queueing_metadata.enq_timestamp") bit<32> enq_timestamp;
    @alias("queueing_metadata.enq_qdepth")    bit<19> enq_qdepth;
    @alias("queueing_metadata.deq_timedelta") bit<32> deq_timedelta;
    @alias("queueing_metadata.deq_qdepth")    bit<19> deq_qdepth;
    // intrinsic metadata
    @alias("intrinsic_metadata.ingress_global_timestamp") bit<48> ingress_global_timestamp;
    @alias("intrinsic_metadata.egress_global_timestamp") bit<48> egress_global_timestamp;
    @alias("intrinsic_metadata.lf_field_list") bit<32> lf_field_list;
    @alias("intrinsic_metadata.mcast_grp")     bit<16> mcast_grp;
    @alias("intrinsic_metadata.resubmit_flag") bit<32> resubmit_flag;
    @alias("intrinsic_metadata.egress_rid")    bit<16> egress_rid;
    /// Indicates that a verify_checksum() method has failed.
    bit<1>  checksum_error;
    @alias("intrinsic_metadata.recirculate_flag") bit<32> recirculate_flag;
}

enum CounterType {
    packets,
    bytes,
    packets_and_bytes
}

enum MeterType {
    packets,
    bytes
}

extern counter {
    counter(bit<32> size, CounterType type);
    void count(in bit<32> index);
}

extern direct_counter {
    direct_counter(CounterType type);
    void count();
}

extern meter {
    meter(bit<32> size, MeterType type);
    void execute_meter<T>(in bit<32> index, out T result);
}

extern direct_meter<T> {
    direct_meter(MeterType type);
    void read(out T result);
}

extern register<T> {
    register(bit<32> size);
    void read(out T result, in bit<32> index);
    void write(in bit<32> index, in T value);
}

// used as table implementation attribute
extern action_profile {
    action_profile(bit<32> size);
}

// Get a random number in the range lo..hi
extern void random<T>(out T result, in T lo, in T hi);
// If the type T is a named struct, the name is used
// to generate the control-plane API.
extern void digest<T>(in bit<32> receiver, in T data);

enum HashAlgorithm {
    crc32,
    crc32_custom,
    crc16,
    crc16_custom,
    random,
    identity,
    csum16,
    xor16
}

extern void mark_to_drop();
extern void hash<O, T, D, M>(out O result, in HashAlgorithm algo, in T base, in D data, in M max);

extern action_selector {
    action_selector(HashAlgorithm algorithm, bit<32> size, bit<32> outputWidth);
}

enum CloneType {
    I2E,
    E2E
}

@deprecated("Please use verify_checksum/update_checksum instead.")
extern Checksum16 {
    Checksum16();
    bit<16> get<D>(in D data);
}

/**
Verifies the checksum of the supplied data.
If this method detects that a checksum of the data is not correct it
sets the standard_metadata checksum_error bit.
@param T          Must be a tuple type where all the fields are bit-fields or varbits.
                  The total dynamic length of the fields is a multiple of the output size.
@param O          Checksum type; must be bit<X> type.
@param condition  If 'false' the verification always succeeds.
@param data       Data whose checksum is verified.
@param checksum   Expected checksum of the data; note that is must be a left-value.
@param algo       Algorithm to use for checksum (not all algorithms may be supported).
                  Must be a compile-time constant.
*/
extern void verify_checksum<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);
/**
Computes the checksum of the supplied data.
@param T          Must be a tuple type where all the fields are bit-fields or varbits.
                  The total dynamic length of the fields is a multiple of the output size.
@param O          Output type; must be bit<X> type.
@param condition  If 'false' the checksum is not changed
@param data       Data whose checksum is computed.
@param checksum   Checksum of the data.
@param algo       Algorithm to use for checksum (not all algorithms may be supported).
                  Must be a compile-time constant.
*/
extern void update_checksum<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);

/**
Verifies the checksum of the supplied data including the payload.
The payload is defined as "all bytes of the packet which were not parsed by the parser".
If this method detects that a checksum of the data is not correct it
sets the standard_metadata checksum_error bit.
@param T          Must be a tuple type where all the fields are bit-fields or varbits.
                  The total dynamic length of the fields is a multiple of the output size.
@param O          Checksum type; must be bit<X> type.
@param condition  If 'false' the verification always succeeds.
@param data       Data whose checksum is verified.
@param checksum   Expected checksum of the data; note that is must be a left-value.
@param algo       Algorithm to use for checksum (not all algorithms may be supported).
                  Must be a compile-time constant.
*/
extern void verify_checksum_with_payload<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);
/**
Computes the checksum of the supplied data including the payload.
The payload is defined as "all bytes of the packet which were not parsed by the parser".
@param T          Must be a tuple type where all the fields are bit-fields or varbits.
                  The total dynamic length of the fields is a multiple of the output size.
@param O          Output type; must be bit<X> type.
@param condition  If 'false' the checksum is not changed
@param data       Data whose checksum is computed.
@param checksum   Checksum of the data.
@param algo       Algorithm to use for checksum (not all algorithms may be supported).
                  Must be a compile-time constant.
*/
extern void update_checksum_with_payload<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);

extern void resubmit<T>(in T data);
extern void recirculate<T>(in T data);
extern void clone(in CloneType type, in bit<32> session);
extern void clone3<T>(in CloneType type, in bit<32> session, in T data);

extern void truncate(in bit<32> length);

// The name 'standard_metadata' is reserved

// Architecture.
// M should be a struct of structs
// H should be a struct of headers or stacks

parser Parser<H, M>(packet_in b,
                    out H parsedHdr,
                    inout M meta,
                    inout standard_metadata_t standard_metadata);

/* The only legal statements in the implementation of the
VerifyChecksum control are: block statements, calls to the
verify_checksum and verify_checksum_with_payload methods,
and return statements. */
control VerifyChecksum<H, M>(inout H hdr,
                             inout M meta);
@pipeline
control Ingress<H, M>(inout H hdr,
                      inout M meta,
                      inout standard_metadata_t standard_metadata);
@pipeline
control Egress<H, M>(inout H hdr,
                     inout M meta,
                     inout standard_metadata_t standard_metadata);

/* The only legal statements in the implementation of the
ComputeChecksum control are: block statements, calls to the
update_checksum and update_checksum_with_payload methods,
and return statements. */
control ComputeChecksum<H, M>(inout H hdr,
                              inout M meta);
@deparser
control Deparser<H>(packet_out b, in H hdr);

package V1Switch<H, M>(Parser<H, M> p,
                       VerifyChecksum<H, M> vr,
                       Ingress<H, M> ig,
                       Egress<H, M> eg,
                       ComputeChecksum<H, M> ck,
                       Deparser<H> dep
                       );

#endif  /* _V1_MODEL_P4_ */
