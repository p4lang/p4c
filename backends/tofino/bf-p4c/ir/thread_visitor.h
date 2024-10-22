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

#ifndef BF_P4C_IR_THREAD_VISITOR_H_
#define BF_P4C_IR_THREAD_VISITOR_H_

#include "ir/ir.h"

class ThreadVisitor : public virtual Visitor {
    friend class IR::BFN::Pipe;
    gress_t thread;

 public:
    explicit ThreadVisitor(gress_t th) : thread(th) {}
    friend gress_t VisitingThread(ThreadVisitor *v) { return v->thread; }
};

extern gress_t VisitingThread(const Visitor *v);

#endif /* BF_P4C_IR_THREAD_VISITOR_H_ */
