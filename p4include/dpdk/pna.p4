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
#include <_internal/pna/v0_7/types_misc.p4>

// #include <_internal/pna/v0_7/extern_hash.p4>

// BEGIN:Hash_algorithms
enum PNA_HashAlgorithm_t {
  // TBD what this type's values will be for PNA
  IDENTITY,
  CRC32,
  CRC32_CUSTOM,
  CRC16,
  CRC16_CUSTOM,
  ONES_COMPLEMENT16,  /// One's complement 16-bit sum used for IPv4 headers,
                      /// TCP, and UDP.
  TOEPLITZ,
  TARGET_DEFAULT      /// target implementation defined
}
// END:Hash_algorithms

// BEGIN:Hash_extern
extern Hash<O> {
  /// Constructor
  Hash(PNA_HashAlgorithm_t algo);

  /// Compute the hash for data.
  /// @param data The data over which to calculate the hash.
  /// @return The hash value.
  O get_hash<D>(in D data);

  /// Compute the hash for data, with modulo by max, then add base.
  /// @param base Minimum return value.
  /// @param data The data over which to calculate the hash.
  /// @param max The hash value is divided by max to get modulo.
  ///        An implementation may limit the largest value supported,
  ///        e.g. to a value like 32, or 256, and may also only
  ///        support powers of 2 for this value.  P4 developers should
  ///        limit their choice to such values if they wish to
  ///        maximize portability.
  /// @return (base + (h % max)) where h is the hash value.
  O get_hash<T, D>(in T base, in D data, in T max);
}
// END:Hash_extern

#include <_internal/pna/v0_7/extern_checksum.p4>

#include <_internal/pna/v0_7/extern_counter.p4>

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
  // dpdk does not support this overload currently if used with
  // BYTES counter type
  void count(in S index);

  // Dpdk support this overload and requires packet length
  void count(in S index, in bit<32> pkt_len);
}
// END:Counter_extern

// BEGIN:DirectCounter_extern
@noWarn("unused")
extern DirectCounter<W> {
  DirectCounter(PNA_CounterType_t type);
  // dpdk does not support this overload currently if used with
  // BYTES counter type
  void count();
  // Dpdk support this overload and requires packet length
  void count(in bit<32> pkt_len);
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
  // dpdk does not support below two execute overload, using it
  // result in compilation failure and suggest using `dpdk_execute`

  // Use this method call to perform a color aware meter update (see
  // RFC 2698). The color of the packet before the method call was
  // made is specified by the color parameter.
  PNA_MeterColor_t execute(in S index, in PNA_MeterColor_t color);
  // Use this method call to perform a color blind meter update (see
  // RFC 2698).  It may be implemented via a call to execute(index,
  // MeterColor_t.GREEN), which has the same behavior.
  PNA_MeterColor_t execute(in S index);

  // Adding param `in bit<32> pkt_len` to execute (part of Meter extern)
  // leads to overload resolution failure due to ambiguous candidates,
  // as p4c do overload resolution based on number of parameter and does
  // not consider types. To workaround this we introduced new method in
  // Meter extern `dpdk_execute` which has extra param.
  PNA_MeterColor_t dpdk_execute(in S index, in PNA_MeterColor_t color, in bit<32> pkt_len);
  PNA_MeterColor_t dpdk_execute(in S index, in bit<32> pkt_len);
}
// END:Meter_extern

// BEGIN:DirectMeter_extern
extern DirectMeter {
  DirectMeter(PNA_MeterType_t type);
  // See the corresponding methods for extern Meter.
  // dpdk does not support below two execute overload, using it
  // result in compilation failure and suggest using `dpdk_execute`
  PNA_MeterColor_t execute(in PNA_MeterColor_t color);
  PNA_MeterColor_t execute();

  PNA_MeterColor_t dpdk_execute(in PNA_MeterColor_t color, in bit<32> pkt_len);
  PNA_MeterColor_t dpdk_execute(in bit<32> pkt_len);
}
// END:DirectMeter_extern

#include <_internal/pna/v0_7/extern_register.p4>
#include <_internal/pna/v0_7/extern_random.p4>
#include <_internal/pna/v0_7/extern_action.p4>
#include <_internal/pna/v0_7/extern_digest.p4>

// PNA extern for IPsec:
// IPsec accelerator result for the current packet
enum ipsec_status {
	IPSEC_SUCCESS,
	IPSEC_ERROR
}

// IPsec accelerator extern definition
extern ipsec_accelerator {
	// IPsec accelerator constructor.
	ipsec_accelerator();

	// Set the security association (SA) index for the current packet.
	//
	// When not invoked, the SA index to be used is undefined, leading to taget dependent
	// behavior.
	void set_sa_index<T>(in T sa_index);

	// Set the offset to the IPv4/IPv6 header for the current packet.
	//
	// When not invoked, the default value set for the IPv4/IPv6 header offset is 0.
	void set_ip_header_offset<T>(in T offset);

	// Enable the IPsec operation to be performed on the current packet.
	//
	// The type of the operation (i.e. encrypt/decrypt) and its associated parameters (such as
	// the crypto cipher and/or authentication parameters, the tunnel/transport mode headers,
	// etc) are completely specified by the SA that was set for the current packet.
	//
	// When enabled, this operation takes place once the deparser operation is completed.
	void enable();

	// Disable any IPsec operation that might have been previously enabled for the current
	// packet.
	void disable();

	// Returns true when the current packet has been processed by the IPsec accelerator and
	// reinjected back into the pipeline, and false otherwise.
	bool from_ipsec(out ipsec_status status);
}
//END:IPSec extern

#include <_internal/pna/v0_5/types_metadata.p4>
#include <_internal/pna/v0_5/extern_funcs.p4>

extern void recirculate();

#include <_internal/pna/v0_5/blocks.p4>

#endif   // __PNA_P4__
