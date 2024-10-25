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
