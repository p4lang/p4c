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

#include "move_to_egress.h"

#include "ir/dump.h"

class MoveToEgress::FindIngressPacketMods : public Visitor {
    MoveToEgress &self;

    void apply();
    const IR::Node *apply_visitor(const IR::Node *n, const char * = 0) override {
        apply();
        return n;
    }

 public:
    explicit FindIngressPacketMods(MoveToEgress &self) : self(self) {}
    FindIngressPacketMods *clone() const override { return new FindIngressPacketMods(*this); }
};

MoveToEgress::MoveToEgress(BFN::EvaluatorPass *ev)
    : PassManager({
          ev,
          [this]() {
              auto toplevel = evaluator->getToplevelBlock();
              auto main = toplevel->getMain();
              for (auto &pkg : main->constantValue) {
                  LOG2(pkg.first->toString()
                       << " : " << (pkg.second ? pkg.second->toString() : "nullptr"));
                  if (auto pb = pkg.second ? pkg.second->to<IR::PackageBlock>() : nullptr) {
                      for (auto &p : pb->constantValue) {
                          LOG2("  " << p.first->toString() << " : "
                                    << (p.second ? p.second->toString() : "nullptr"));
                          if (p.second) {
                              auto name = p.first->toString();
                              if (name == "ingress_parser")
                                  ingress_parser.insert(p.second->to<IR::ParserBlock>()->container);
                              else if (name == "ingress")
                                  ingress.insert(p.second->to<IR::ControlBlock>()->container);
                              else if (name == "ingress_deparser")
                                  ingress_deparser.insert(
                                      p.second->to<IR::ControlBlock>()->container);
                              else if (name == "egress_parser")
                                  egress_parser.insert(p.second->to<IR::ParserBlock>()->container);
                              else if (name == "egress")
                                  egress.insert(p.second->to<IR::ControlBlock>()->container);
                              else if (name == "egress_deparser")
                                  egress_deparser.insert(
                                      p.second->to<IR::ControlBlock>()->container);
                          }
                      }
                  }
              }
          },
          [this](const IR::Node *root) {
              if (LOGGING(5))
                  dump(::Log::Detail::fileLogOutput(__FILE__), root);
              else
                  LOG4(*root);
          },
          &defuse,
          FindIngressPacketMods(*this),
      }),
      evaluator(ev) {}

void MoveToEgress::FindIngressPacketMods::apply() {
    for (auto *ic : self.ingress) {
        auto params = ic->getApplyParameters();
        auto hdrs = params->parameters.at(0);
        for (auto def : self.defuse.getDefs(hdrs)) {
            if (def->node != hdrs) {
                warning("%s%s modified in %s", def->node->srcInfo, hdrs, ic);
            }
        }
    }
}
