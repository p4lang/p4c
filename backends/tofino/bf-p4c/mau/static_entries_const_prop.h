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

#ifndef BF_P4C_MAU_STATIC_ENTRIES_CONST_PROP_H_
#define BF_P4C_MAU_STATIC_ENTRIES_CONST_PROP_H_

#include "bf-p4c/mau/mau_visitor.h"

using namespace P4;

/*
  In P4-16, user can specify constant static entries for table matches.
  We can use these static entries to do constant propagation.
  In example below, the assignment "hdr.b = hdr.a" introduces a header
  validity assignment "hdr.b.$valid = hdr.a.$valid", which can be safely
  reduced to "hdr.b.$valid = true". This reduces the instruction from
  a two source instruction to a single source one, which is much friendly
  for POV bits packing.

      action rewrite_b() {
          hdr.b = hdr.a;
          hdr.a.setInvalid();
      }

      table decap {
          key = {
              hdr.a.isValid() : exact;
          }

          actions = {
              rewrite_b;
          }

          const entries = {
              { true } : rewrite_b();
          }
      }

  The same applies for keyless table that is predicated by a gateway TODO

      table decap {
          actions = {
              rewrite_b;
          }
      }

      if (hdr.a.isValid()) {
          decap.apply();
      }
*/

class StaticEntriesConstProp : public MauModifier {
    const PhvInfo &phv;

 public:
    explicit StaticEntriesConstProp(const PhvInfo &phv) : phv(phv) {}

    bool equiv(const IR::MethodCallExpression *mc, const IR::MAU::Action *ac) {
        if (mc && ac) {
            if (auto path = mc->method->to<IR::PathExpression>()) {
                if (path->path->name == ac->name) {
                    return true;
                }
            }
        }
        return false;
    }

    bool preorder(IR::MAU::Instruction *instr) override {
        auto table = findContext<IR::MAU::Table>();
        if (table->is_compiler_generated || table->entries_list == nullptr) return false;

        auto action = findContext<IR::MAU::Action>();

        for (unsigned i = 0; i < instr->operands.size(); i++) {
            if (i == 0) continue;

            // Must be PHV. See bug check number 2 in verify_conditional_set_without_phv()
            if ((i == 1) && (instr->name == "conditionally-set")) {
                continue;
            }

            auto op = phv.field(instr->operands[i]);
            if (!op) continue;

            for (unsigned j = 0; j < table->match_key.size(); j++) {
                if (table->match_key[j]->match_type.name != "exact") continue;

                auto key = phv.field(table->match_key[j]->expr);
                if (op == key) {
                    bool can_const_prop = false;
                    const IR::Expression *match = nullptr;
                    // Check if constant values for one same action in multi-entries are unique.
                    // Don't do constant propagation if not unique or it's a default action.
                    for (auto &ent : table->entries_list->entries) {
                        if (equiv(ent->action->to<IR::MethodCallExpression>(), action) &&
                            (action->init_default == false)) {
                            if (can_const_prop == false) {
                                // Assume can constant propagtion for the first entry.
                                match = ent->keys->components.at(j);
                                can_const_prop = true;
                            } else {
                                // Need to check if more than 1 entry with the same action.
                                auto cval = ent->keys->components.at(j);
                                if (!match->equiv(*cval)) {
                                    can_const_prop = false;
                                    break;
                                }
                            }
                        }
                    }

                    if (can_const_prop) {
                        LOG4("rewrite " << instr);
                        if (auto b = match->to<IR::BoolLiteral>()) {
                            instr->operands[i] = new IR::Constant(IR::Type::Bits::get(1), b->value);
                        } else if (auto c = match->to<IR::Constant>()) {
                            instr->operands[i] = c;
                        } else {
                            LOG4("unknown static entry match type");
                        }
                        LOG4("as " << instr);
                    }
                }
            }
        }

        return false;
    }
};

#endif /* BF_P4C_MAU_STATIC_ENTRIES_CONST_PROP_H_ */
