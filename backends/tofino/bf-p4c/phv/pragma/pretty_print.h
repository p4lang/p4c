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

#ifndef BF_P4C_PHV_PRAGMA_PRETTY_PRINT_H_
#define BF_P4C_PHV_PRAGMA_PRETTY_PRINT_H_

#include <string>
// Pretty-print Pragma classes for debugging and logging purpose

namespace Pragma {

// defines a common interface for pretty printing collected pragmas, also a
// place to hold common pretty print routines if there is any.
class PrettyPrint {
 public:
    virtual std::string pretty_print() const = 0;
};

}  // namespace Pragma

#endif /* BF_P4C_PHV_PRAGMA_PRETTY_PRINT_H_ */
