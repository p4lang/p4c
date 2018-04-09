/* -*- P4_16 -*- */

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

/* P4-16 declaration of the P4-D2 (P4 Developer's Day) switch model */

#include <core.p4>

/***************************************************************************
 *************** C O M M O N   P O R T A B L E   T Y P E S *****************
 ***************        A N D    C O N S T A N T S         *****************
 ***************************************************************************/
typedef bit<9>   portid_t;           /* Port ID -- ingress or egress port  */
typedef bit<16>  mgid_t;             /* Multicast Group ID  */
typedef bit<16>  cloneid_t;          /* Clone ID            */
typedef bit<16>  rid_t;              /* Replication ID      */
typedef bit<5>   qid_t;              /* Queue ID            */
typedef bit<48>  timestamp_t;
typedef bit<16>  queue_depth_t;
typedef bit<16>  packet_length_t;
typedef bit<16>  queue_id_t;

/* Additional, architecture-specific match kinds */
match_kind {
    range
}

/****************************************************************************
 ********** S T A N D A R D  I N T R I N S I C   M E T A D A T A ************
 ****************************************************************************/
struct standard_metadata_t {
    portid_t    ingress_port;   /* The packet the port ingressed in         */
    portid_t    egress_port;    /* Used of mcast_group != 0                 */
    mgid_t      mcast_group;    /* Non-zero value specifies multicast group */
    bit<1>      resubmit_flag;  /* The resubmit() indicator                 */
    bit<1>      drop_flag;      /* Packet (but not clone) is dropped        */
    cloneid_t   clone_id;       /* Non-zero value identifies clone_i2e dest */
    qid_t       egress_queue;   /* Requested Queue to hold the packet in    */
}

/****************************************************************************
 ** C H E C K S U M   V E R I F I E R  I N T R I N S I C   M E T A D A T A **
 ****************************************************************************/

/****************************************************************************
 ********** I N G R E S S   I N T R I N S I C   M E T A D A T A *************
 ****************************************************************************/
struct ingress_input_t {
    timestamp_t    ingress_timestamp; /* Ingress timestamp                  */
}

/****************************************************************************
 **********  E G R E S S   I N T R I N S I C   M E T A D A T A  *************
 ****************************************************************************/
struct egress_input_t {
    rid_t             egress_rid;     /* Multicast RID (0 for unicast)      */
    packet_length_t   packet_length;  /* Packet length in bytes             */

    timestamp_t       enq_timestamp;  /* The time the packet was enqueued   */
    timestamp_t       deq_timedelta;  /* The time spent in the queue        */
    queue_depth_t     enq_qdepth;     /* Q depth when packet was enqueued   */
    queue_depth_t     deq_qdepth;     /* Q depth when packet was dequeued   */
}

/****************************************************************************
 **  C H E C K S U M   U P D A T E R   I N T R I N S I C   M E T A D A T A **
 ****************************************************************************/

/***************************************************************************
 ********* D E P A R S E R  I N T R I N S I C   M E T A D A T A ************
 ***************************************************************************/

/***************************************************************************
 ********** S T A T E F U L   E X T E R N   O B J E C T S  *****************
 ***************************************************************************/

/************************ Header Checksum Units ****************************/
extern Checksum16 {
    Checksum16();
    bit<16> get<D>(in D data);
}

/******************************* Counters **********************************/
enum counter_type_t {
    packets,
    bytes,
    packets_and_bytes
}

extern counter<T> {
    counter(T instance_count, counter_type_t type);
    void count(in T index);
}

/* Must be referecened in the table declaration via counters= attribute */
extern direct_counter {
    direct_counter(counter_type_t type);
    void count();
}

/******************************** Meters ***********************************/
enum meter_type_t {
    packets,
    bytes
}

typedef bit<2> meter_color_t;
const meter_color_t METER_COLOR_GREEN  = 0;
const meter_color_t METER_COLOR_YELLOW = 1;
const meter_color_t METER_COLOR_RED    = 2;

extern meter<T> {
    meter(T instance_count, meter_type_t type);
    meter_color_t execute_meter(in T index);
}

/* Must be referecened in the table declaration via meters= attribute */
extern direct_meter {
    direct_meter(meter_type_t type);
    meter_color_t execute_meter();
}

/****************************** Registers **********************************/
extern register<T, I> {
    register(I instance_count);
    T read(in I index);
    void write(in I index, in T value);
}

/********************** Random Number Generators ***************************/
extern random<T> {
    random(T lo, T hi);
    T get();
}

/******************** Hash (CRC) Calculation Units *************************/
enum hash_algorithm_t {
    identity,
    crc16,
    crc32,
    random,
    custom
}

extern hash<T, D> {
    hash(hash_algorithm_t algo);
    T compute(in D data, in T base, in T max);
}

/************************* Digest Generation ******************************/
typedef bit<32> digest_receiver_t;
extern digest<T> {
    digest(digest_receiver_t receiver);
    void generate(in T data);
}

/***************************************************************************
 ************** C O N V E N I E N C E  F U N C T I O N S ********************
 ***************************************************************************/

/*
 * Note: These actions assume that the parameters in the user-instantiated
 *       controls are named the same way as in the architecture definition
 */
#define mark_for_drop() { standard_metadata.drop_bit = 1; }

#define drop() { mark_for_drop(standard_metadata); exit; }

#define resubmit() { standard_metadata.resubmit_flag = 1; }

#define clone_to_egress(clone_id) { standard_metadata.clone_id = clone_id; }

/***************************************************************************
 ***************** P A C K A G E   C O M P O N E N T S  ********************
 ***************************************************************************/

parser Parser<H, M>(packet_in b,
                    out   H               parsedHdr,
                    inout M               meta,
                    inout standard_metadata_t standard_meta);

control VerifyChecksum<H,M>(in    H hdr,
                            inout M meta);

@pipeline
control Ingress<H, M>(inout H                hdr,
                      inout M                meta,
                      in    error            parser_error,
                      in    ingress_input_t  input_meta,
                      inout standard_metadata_t output_meta);

@pipeline
control Egress<H, M>(inout H               hdr,
                     inout M               meta,
                     in    egress_input_t  input_meta,
                     inout standard_metadata_t output_meta);

control ComputeChecksum<H, M>(inout H hdr,
                              inout M meta);

@deparser
control Deparser<H>(packet_out b, in H hdr);

package P4D2Switch<H, M>(Parser<H, M>          user_parser,
                         VerifyChecksum<H, M>  user_checksum_verification,
                         Ingress<H, M>         user_ingress,
                         Egress<H, M>          user_egress,
                         ComputeChecksum<H, M> user_checksum_compute,
                         Deparser<H>           user_deparser
                        );
