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

#ifndef BF_P4C_PHV_ERROR_H_
#define BF_P4C_PHV_ERROR_H_

#include <sstream>

namespace PHV {

/// ErrorCode is the specific reason of failure of function invoke.
enum class ErrorCode {
    ok = 0,
    unknown,
    // utils::is_well_formed
    slicelist_sz_mismatch,
};

/// Error represents an error of internal phv
/// function call. It stores a string of reason for debug
/// and an error code.
class Error : public std::stringstream {
 public:
    ErrorCode code;
    Error() : code(ErrorCode::ok) {}
    void set(ErrorCode c) { code = c; }
    bool is(ErrorCode c) { return code == c; }
};

}  // namespace PHV

#endif /* BF_P4C_PHV_ERROR_H_ */
