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

#ifndef BF_P4C_MIDEND_MOVE_TO_EGRESS_H_
#define BF_P4C_MIDEND_MOVE_TO_EGRESS_H_

#include "defuse.h"
#include "ir/ir.h"
#include "midend/type_checker.h"

class MoveToEgress : public PassManager {
    BFN::EvaluatorPass *evaluator;
    ordered_set<const IR::P4Parser *> ingress_parser, egress_parser;
    ordered_set<const IR::P4Control *> ingress, egress, ingress_deparser, egress_deparser;
    ComputeDefUse defuse;

    class FindIngressPacketMods;

 public:
    explicit MoveToEgress(BFN::EvaluatorPass *ev);
};

#endif /* BF_P4C_MIDEND_MOVE_TO_EGRESS_H_ */
