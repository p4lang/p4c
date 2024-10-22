/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
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
inline gress_t operator~(const gress_t &gress) { return gress_t((gress & 1) ^ 1); }

cstring toString(gress_t gress);
cstring toSymbol(gress_t gress);
cstring createThreadName(gress_t gress, cstring name);
cstring stripThreadPrefix(cstring name);

std::ostream &operator<<(std::ostream &out, gress_t gress);
std::ostream &operator<<(std::ostream &out, std::optional<gress_t> gress);
bool operator>>(cstring s, gress_t &gressOut);

#endif /* BF_P4C_IR_GRESS_H_ */
