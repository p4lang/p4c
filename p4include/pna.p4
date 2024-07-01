/* Copyright 2020-present Intel Corporation

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

/**************************************************************
 * PNA specification is work-in-progress, this file is subject
 * to change without any notification. Use at your own risk.
 **************************************************************/

#ifndef __PNA_P4__
#define __PNA_P4__

#include <core.p4>

/**
 *   P4-16 declaration of the Portable NIC Architecture
 */

#include <_internal/pna/v0_5/types_core.p4>
// #include <_internal/pna/v0_5/types_defns.p4>
#include <_internal/pna/v0_5/types_defns2.p4>
#include <_internal/pna/v0_5/funcs_int_to_header.p4>
#include <_internal/pna/v0_5/types_misc.p4>

#include <_internal/pna/v0_5/extern_hash.p4>
#include <_internal/pna/v0_7/extern_checksum.p4>

// #include <_internal/pna/v0_7/extern_counter.p4>

// BEGIN:CounterType_defn
enum PNA_CounterType_t {
    PACKETS,
    BYTES,
    PACKETS_AND_BYTES
}
// END:CounterType_defn

// BEGIN:Counter_extern
/// Indirect counter with n_counters independent counter values, where
/// every counter value has a data plane size specified by type W.

@noWarn("unused")
extern Counter<W, S> {
  Counter(bit<32> n_counters, PNA_CounterType_t type);
  void count(in S index, @optional in bit<32> increment);
}
// END:Counter_extern

// BEGIN:DirectCounter_extern
@noWarn("unused")
extern DirectCounter<W> {
  DirectCounter(PNA_CounterType_t type);
  void count(@optional in bit<32> increment);
}
// END:DirectCounter_extern

// #include <_internal/pna/v0_7/extern_meter.p4>

// BEGIN:MeterType_defn
enum PNA_MeterType_t {
    PACKETS,
    BYTES
}
// END:MeterType_defn

// BEGIN:MeterColor_defn
enum PNA_MeterColor_t { RED, GREEN, YELLOW }
// END:MeterColor_defn

// BEGIN:Meter_extern
// Indexed meter with n_meters independent meter states.

extern Meter<S> {
  Meter(bit<32> n_meters, PNA_MeterType_t type);

  // Use this method call to perform a color aware meter update (see
  // RFC 2698). The color of the packet before the method call was
  // made is specified by the color parameter.
  PNA_MeterColor_t execute(in S index, in PNA_MeterColor_t color);

  // Use this method call to perform a color blind meter update (see
  // RFC 2698).  It may be implemented via a call to execute(index,
  // MeterColor_t.GREEN), which has the same behavior.
  PNA_MeterColor_t execute(in S index);
}
// END:Meter_extern

// BEGIN:DirectMeter_extern
extern DirectMeter {
  DirectMeter(PNA_MeterType_t type);
  // See the corresponding methods for extern Meter.
  PNA_MeterColor_t execute(in PNA_MeterColor_t color, @optional in bit<32> pkt_len);
  PNA_MeterColor_t execute(@optional in bit<32> pkt_len);
}
// END:DirectMeter_extern

#include <_internal/pna/v0_7/extern_register.p4>
#include <_internal/pna/v0_7/extern_random.p4>
#include <_internal/pna/v0_7/extern_action.p4>
#include <_internal/pna/v0_7/extern_digest.p4>

#include <_internal/pna/v0_5/types_metadata.p4>
#include <_internal/pna/v0_5/extern_funcs.p4>

extern void recirculate();

#include <_internal/pna/v0_5/blocks.p4>

#endif   // __PNA_P4__
