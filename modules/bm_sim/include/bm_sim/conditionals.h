/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef _BM_CONDITIONALS_H_
#define _BM_CONDITIONALS_H_

#include <unordered_map>
#include <string>
#include <vector>

#include "data.h"
#include "phv.h"
#include "control_flow.h"
#include "event_logger.h"
#include "named_p4object.h"

enum class ExprOpcode {
  LOAD_FIELD, LOAD_HEADER, LOAD_BOOL, LOAD_CONST,
  ADD, SUB, MOD,
  EQ_DATA, NEQ_DATA, GT_DATA, LT_DATA, GET_DATA, LET_DATA,
  AND, OR, NOT,
  BIT_AND, BIT_OR, BIT_XOR, BIT_NEG,
  VALID_HEADER
};

class ExprOpcodesMap {
public:
  static ExprOpcode get_opcode(std::string expr_name);

private:
  static ExprOpcodesMap *get_instance();

  ExprOpcodesMap();

private:
  std::unordered_map<std::string, ExprOpcode> opcodes_map{};
};

struct Op {
  ExprOpcode opcode;

  union {
    int data_dest_index;
    
    struct {
      header_id_t header;
      int field_offset;
    } field;

    header_id_t header;

    bool bool_value;

    int const_offset;
  };
};

class Conditional : public ControlFlowNode, public NamedP4Object {
public:
  Conditional(const std::string &name, p4object_id_t id)
    : NamedP4Object(name, id) {}

  void push_back_load_field(header_id_t header, int field_offset);
  void push_back_load_bool(bool value);
  void push_back_load_header(header_id_t header);
  void push_back_load_const(const Data &data);
  void push_back_op(ExprOpcode opcode);

  void build();

  bool eval(const PHV &phv) const;

  void set_next_node_if_true(ControlFlowNode *next_node) {
    true_next = next_node;
  }

  void set_next_node_if_false(ControlFlowNode *next_node) {
    false_next = next_node;
  }

  // return pointer to next control flow node
  const ControlFlowNode *operator()(Packet *pkt) const override;

  Conditional(const Conditional &other) = delete;
  Conditional &operator=(const Conditional &other) = delete;

  Conditional(Conditional &&other) /*noexcept*/ = default;
  Conditional &operator=(Conditional &&other) /*noexcept*/ = default;

private:
  int assign_dest_registers();
  
private:
  std::vector<Op> ops{};
  std::vector<Data> const_values{};
  ControlFlowNode *true_next{nullptr};
  ControlFlowNode *false_next{nullptr};
  int data_registers_cnt{0};
  bool built{false};
};

#endif
