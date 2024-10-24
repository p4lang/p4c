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
