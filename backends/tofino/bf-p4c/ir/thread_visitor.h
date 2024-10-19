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
