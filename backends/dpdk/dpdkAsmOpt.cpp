/*
Copyright 2020 Intel Corp.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "dpdkAsmOpt.h"

namespace DPDK {
// One assumption of this piece of program is that DPDK never produces jmp
// statements that jump back to previous statements.
const IR::Node *RemoveRedundantLabel::postorder(IR::DpdkListStatement *l) {
  bool changed = false;
  IR::IndexedVector<IR::DpdkAsmStatement> used_labels;
  for (auto stmt : l->statements) {
    if (auto jmp = stmt->to<IR::DpdkJmpBaseStatement>()) {
      bool found = false;
      for (auto label : used_labels) {
        if (label->to<IR::DpdkJmpBaseStatement>()->label == jmp->label) {
          found = true;
          break;
        }
      }
      if (!found) {
        used_labels.push_back(stmt);
      }
    }
  }
  auto new_l = new IR::IndexedVector<IR::DpdkAsmStatement>;
  for (auto stmt : l->statements) {
    if (auto label = stmt->to<IR::DpdkLabelStatement>()) {
      bool found = false;
      for (auto jmp_label : used_labels) {
        if (jmp_label->to<IR::DpdkJmpBaseStatement>()->label == label->label) {
          found = true;
          break;
        }
      }
      if (found) {
        new_l->push_back(stmt);
      } else {
        changed = true;
      }
    } else {
      new_l->push_back(stmt);
    }
  }
  if (changed)
    l->statements = *new_l;
  return l;
}

const IR::Node *RemoveUselessJmpAndLabel::postorder(IR::DpdkListStatement *l) {
  const IR::DpdkJmpBaseStatement *cache = nullptr;
  bool changed = false;
  IR::IndexedVector<IR::DpdkAsmStatement> new_l;
  for (auto stmt : l->statements) {
    if (auto jmp = stmt->to<IR::DpdkJmpBaseStatement>()) {
      if (cache)
        new_l.push_back(cache);
      cache = jmp;
    } else if (auto label = stmt->to<IR::DpdkLabelStatement>()) {
      if (!cache) {
        new_l.push_back(stmt);
      } else if (cache->label != label->label) {
        new_l.push_back(cache);
        cache = nullptr;
        new_l.push_back(stmt);
      } else {
        new_l.push_back(stmt);
        cache = nullptr;
        changed = true;
      }
    } else {
      if (cache) {
        new_l.push_back(cache);
        cache = nullptr;
      }
      new_l.push_back(stmt);
    }
  }
  if (changed)
    l->statements = new_l;
  return l;
}

const IR::Node *RemoveJmpAfterLabel::postorder(IR::DpdkListStatement *l) {
  bool changed = false;
  std::map<const cstring, cstring> label_map;
  const IR::DpdkLabelStatement *cache = nullptr;
  for (auto stmt : l->statements) {
    if (!cache) {
      if (auto label = stmt->to<IR::DpdkLabelStatement>()) {
        cache = label;
      }
    } else {
      if (auto jmp = stmt->to<IR::DpdkJmpStatement>()) {
        label_map.emplace(cache->label, jmp->label);
      } else {
        cache = nullptr;
      }
    }
  }
  auto new_l = new IR::IndexedVector<IR::DpdkAsmStatement>;
  for (auto stmt : l->statements) {
    if (auto jmp = stmt->to<IR::DpdkJmpBaseStatement>()) {
      auto res = label_map.find(jmp->label);
      if (res != label_map.end()) {
        ((IR::DpdkJmpBaseStatement *)stmt)->label = res->second;
        new_l->push_back(stmt);
        changed = true;
      } else {
        new_l->push_back(stmt);
      }
    } else {
      new_l->push_back(stmt);
    }
  }
  if (changed)
    l->statements = *new_l;
  return l;
}

const IR::Node *RemoveLabelAfterLabel::postorder(IR::DpdkListStatement *l) {
  bool changed = false;
  std::map<const cstring, cstring> label_map;
  const IR::DpdkLabelStatement *cache = nullptr;
  for (auto stmt : l->statements) {
    if (!cache) {
      if (auto label = stmt->to<IR::DpdkLabelStatement>()) {
        cache = label;
      }
    } else {
      if (auto label = stmt->to<IR::DpdkLabelStatement>()) {
        label_map.emplace(cache->label, label->label);
      } else {
        cache = nullptr;
      }
    }
  }
  auto new_l = new IR::IndexedVector<IR::DpdkAsmStatement>;
  for (auto stmt : l->statements) {
    if (auto jmp = stmt->to<IR::DpdkJmpBaseStatement>()) {
      auto res = label_map.find(jmp->label);
      if (res != label_map.end()) {
        ((IR::DpdkJmpBaseStatement *)stmt)->label = res->second;
        new_l->push_back(stmt);
        changed = true;
      } else {
        new_l->push_back(stmt);
      }
    } else {
      new_l->push_back(stmt);
    }
  }
  if (changed)
    l->statements = *new_l;
  return l;
}

}  // namespace DPDK
