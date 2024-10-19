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

#ifndef BF_P4C_PHV_V2_SORT_MACROS_H_
#define BF_P4C_PHV_V2_SORT_MACROS_H_

/// convenience macros.
#define IF_NEQ_RETURN_IS_LESS(a, b) \
    if ((a) != (b)) return (a) < (b);
#define IF_NEQ_RETURN_IS_GREATER(a, b) \
    if ((a) != (b)) return (a) > (b);

#endif /* BF_P4C_PHV_V2_SORT_MACROS_H_ */
