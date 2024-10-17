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

#ifndef BF_P4C_PHV_CREATE_THREAD_LOCAL_INSTANCES_H_
#define BF_P4C_PHV_CREATE_THREAD_LOCAL_INSTANCES_H_

#include "ir/ir.h"
#include "ir/pass_manager.h"

/**
 * This Visitor creates a thread-local instance of every metadata instance,
 * header instance, parser state, TempVar, padding. The name of the new instance
 * follows the pattern `gress-name::instance-name`.
 *
 * The set of metadata variables in BFN::Pipe::metadata is also updated.
 */
struct CreateThreadLocalInstances : public PassManager {
    CreateThreadLocalInstances();
};

#endif /* BF_P4C_PHV_CREATE_THREAD_LOCAL_INSTANCES_H_ */
