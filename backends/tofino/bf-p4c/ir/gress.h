/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BF_P4C_IR_GRESS_H_
#define BF_P4C_IR_GRESS_H_

#include <iosfwd>
#include <optional>
#include "lib/cstring.h"

using namespace P4;

/// An enumeration identifying a thread in the Tofino architecture.
enum gress_t {
  INGRESS = 0,
  EGRESS = 1,
  GHOST = 2,
  GRESS_T_COUNT  // number of different threads (max of all targets)
};

/// @return, given a thread @gress, the _other_ thread.  Ingress and Ghost are considered the
/// same, so they both return egress.
inline gress_t operator~(const gress_t& gress) { return gress_t((gress & 1) ^ 1); }

cstring toString(gress_t gress);
cstring toSymbol(gress_t gress);
cstring createThreadName(gress_t gress, cstring name);
cstring stripThreadPrefix(cstring name);

std::ostream& operator<<(std::ostream& out, gress_t gress);
std::ostream& operator<<(std::ostream& out, std::optional<gress_t> gress);
bool operator>>(cstring s, gress_t& gressOut);

#endif /* BF_P4C_IR_GRESS_H_ */
