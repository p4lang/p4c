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

#include "thread_visitor.h"

gress_t VisitingThread(const Visitor *v) {
    const Visitor::Context *ctxt = nullptr;
    if (auto *pipe = v->findContext<IR::BFN::Pipe>(ctxt)) {
        // For a pipe with multiple parsers the index to determine gress will
        // depend on the number of parsers used in each gress (max 18 parsers
        // are allowed per gress)
        //  _____________________________________________________________
        // | INGRESS                  | EGRESS                   | GHOST |
        // | P0 ... P17 | MAU | DPRSR | P0 ... P17 | MAU | DPRSR |       |
        //  -------------------------------------------------------------
        auto num_ingress_indices = pipe->thread[INGRESS].parsers.size() + 2;
        auto num_egress_indices = pipe->thread[EGRESS].parsers.size() + 2;
        if (ctxt->child_index < int(num_ingress_indices))
            return INGRESS;
        else if (ctxt->child_index < int(num_ingress_indices + num_egress_indices))
            return EGRESS;
        else
            return GHOST;
    }
    BUG("Not visiting a BFN::Pipe");
}
