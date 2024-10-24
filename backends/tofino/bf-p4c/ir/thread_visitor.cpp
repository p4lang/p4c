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
