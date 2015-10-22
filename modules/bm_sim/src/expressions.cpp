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

#include <stack>

#include <cassert>

#include "bm_sim/expressions.h"
#include "bm_sim/phv.h"

ExprOpcodesMap::ExprOpcodesMap() {
  opcodes_map = {
    {"load_field", ExprOpcode::LOAD_FIELD},
    {"load_header", ExprOpcode::LOAD_HEADER},
    {"load_bool", ExprOpcode::LOAD_BOOL},
    {"load_const", ExprOpcode::LOAD_CONST},
    {"load_local", ExprOpcode::LOAD_LOCAL},
    {"+", ExprOpcode::ADD},
    {"-", ExprOpcode::SUB},
    {"*", ExprOpcode::MUL},
    {"<<", ExprOpcode::SHIFT_LEFT},
    {">>", ExprOpcode::SHIFT_RIGHT},
    {"==", ExprOpcode::EQ_DATA},
    {"!=", ExprOpcode::NEQ_DATA},
    {">", ExprOpcode::GT_DATA},
    {"<", ExprOpcode::LT_DATA},
    {">=", ExprOpcode::GET_DATA},
    {"<=", ExprOpcode::LET_DATA},
    {"and", ExprOpcode::AND},
    {"or", ExprOpcode::OR},
    {"not", ExprOpcode::NOT},
    {"&", ExprOpcode::BIT_AND},
    {"|", ExprOpcode::BIT_OR},
    {"^", ExprOpcode::BIT_XOR},
    {"~", ExprOpcode::BIT_NEG},
    {"valid", ExprOpcode::VALID_HEADER},
  };
}

ExprOpcodesMap *ExprOpcodesMap::get_instance() {
  static ExprOpcodesMap instance;
  return &instance;
}

ExprOpcode ExprOpcodesMap::get_opcode(std::string expr_name) {
  ExprOpcodesMap *instance = get_instance();
  return instance->opcodes_map[expr_name];
}

void Expression::push_back_load_field(header_id_t header, int field_offset) {
  Op op;
  op.opcode = ExprOpcode::LOAD_FIELD;
  op.field = {header, field_offset};
  ops.push_back(op);
}

void Expression::push_back_load_bool(bool value) {
  Op op;
  op.opcode = ExprOpcode::LOAD_BOOL;
  op.bool_value = value;
  ops.push_back(op);
}

void Expression::push_back_load_header(header_id_t header) {
  Op op;
  op.opcode = ExprOpcode::LOAD_HEADER;
  op.header = header;
  ops.push_back(op);
}

void Expression::push_back_load_const(const Data &data) {
  const_values.push_back(data);
  Op op;
  op.opcode = ExprOpcode::LOAD_CONST;
  op.const_offset = const_values.size() - 1;
  ops.push_back(op);
}

void Expression::push_back_load_local(const int offset) {
  Op op;
  op.opcode = ExprOpcode::LOAD_LOCAL;
  op.local_offset = offset;
  ops.push_back(op);
}

void Expression::push_back_op(ExprOpcode opcode) {
  Op op;
  op.opcode = opcode;
  ops.push_back(op);
}

void Expression::build() {
  data_registers_cnt = assign_dest_registers();
  built = true;
}

/* I have made this function more efficient by using thread_local variables
   instead of dynamic allocation at each call. Maybe it would be better to just
   try to use a stack allocator */
void Expression::eval_(const PHV &phv, ExprType expr_type,
		       const std::vector<Data> &locals,
		       bool *b_res, Data *d_res) const {
  assert(built);

  static thread_local int data_temps_size = 4;
  // std::vector<Data> data_temps(data_registers_cnt);
  static thread_local std::vector<Data> data_temps(data_temps_size);
  while(data_temps_size < data_registers_cnt) {
    data_temps.emplace_back();
    data_temps_size++;
  }

  /* Logically, I am using these as stacks but experiments showed that using
     vectors directly was more efficient (also I can call reserve to avoid 
     multiple calls to malloc */

  /* 4 is arbitrary, it is possible to do an analysis on the Expression to find
     the exact number needed, but I don't think it is worth it... */

  static thread_local std::vector<bool> bool_temps_stack;
  bool_temps_stack.clear();
  // bool_temps_stack.reserve(4);

  static thread_local std::vector<const Data *> data_temps_stack;
  data_temps_stack.clear();
  // data_temps_stack.reserve(4);

  static thread_local std::vector<const Header *> header_temps_stack;
  header_temps_stack.clear();
  // header_temps_stack.reserve(4);

  bool lb, rb;
  const Data *ld, *rd;

  for(const auto &op : ops) {
    switch(op.opcode) {
    case ExprOpcode::LOAD_FIELD:
      data_temps_stack.push_back(&(phv.get_field(op.field.header,
						 op.field.field_offset)));
      break;

    case ExprOpcode::LOAD_HEADER:
      header_temps_stack.push_back(&(phv.get_header(op.header)));
      break;

    case ExprOpcode::LOAD_BOOL:
      bool_temps_stack.push_back(op.bool_value);
      break;

    case ExprOpcode::LOAD_CONST:
      data_temps_stack.push_back(&const_values[op.const_offset]);
      break;

    case ExprOpcode::LOAD_LOCAL:
      data_temps_stack.push_back(&locals[op.local_offset]);
      break;

    case ExprOpcode::ADD:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      data_temps[op.data_dest_index].add(*ld, *rd);
      data_temps_stack.push_back(&data_temps[op.data_dest_index]);
      break;

    case ExprOpcode::SUB:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      data_temps[op.data_dest_index].sub(*ld, *rd);
      data_temps_stack.push_back(&data_temps[op.data_dest_index]);
      break;

    case ExprOpcode::MUL:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      data_temps[op.data_dest_index].multiply(*ld, *rd);
      data_temps_stack.push_back(&data_temps[op.data_dest_index]);
      break;

    case ExprOpcode::SHIFT_LEFT:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      data_temps[op.data_dest_index].shift_left(*ld, *rd);
      data_temps_stack.push_back(&data_temps[op.data_dest_index]);
      break;

    case ExprOpcode::SHIFT_RIGHT:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      data_temps[op.data_dest_index].shift_right(*ld, *rd);
      data_temps_stack.push_back(&data_temps[op.data_dest_index]);
      break;

    case ExprOpcode::EQ_DATA:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      bool_temps_stack.push_back(*ld == *rd);
      break;

    case ExprOpcode::NEQ_DATA:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      bool_temps_stack.push_back(*ld != *rd);
      break;

    case ExprOpcode::GT_DATA:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      bool_temps_stack.push_back(*ld > *rd);
      break;

    case ExprOpcode::LT_DATA:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      bool_temps_stack.push_back(*ld < *rd);
      break;

    case ExprOpcode::GET_DATA:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      bool_temps_stack.push_back(*ld >= *rd);
      break;

    case ExprOpcode::LET_DATA:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      bool_temps_stack.push_back(*ld <= *rd);
      break;

    case ExprOpcode::AND:
      rb = bool_temps_stack.back(); bool_temps_stack.pop_back();
      lb = bool_temps_stack.back(); bool_temps_stack.pop_back();
      bool_temps_stack.push_back(lb && rb);
      break;

    case ExprOpcode::OR:
      rb = bool_temps_stack.back(); bool_temps_stack.pop_back();
      lb = bool_temps_stack.back(); bool_temps_stack.pop_back();
      bool_temps_stack.push_back(lb || rb);
      break;

    case ExprOpcode::NOT:
      rb = bool_temps_stack.back(); bool_temps_stack.pop_back();
      bool_temps_stack.push_back(!rb);
      break;

    case ExprOpcode::BIT_AND:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      data_temps[op.data_dest_index].bit_and(*ld, *rd);
      data_temps_stack.push_back(&data_temps[op.data_dest_index]);
      break;

    case ExprOpcode::BIT_OR:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      data_temps[op.data_dest_index].bit_or(*ld, *rd);
      data_temps_stack.push_back(&data_temps[op.data_dest_index]);
      break;

    case ExprOpcode::BIT_XOR:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      ld = data_temps_stack.back(); data_temps_stack.pop_back();
      data_temps[op.data_dest_index].bit_xor(*ld, *rd);
      data_temps_stack.push_back(&data_temps[op.data_dest_index]);
      break;

    case ExprOpcode::BIT_NEG:
      rd = data_temps_stack.back(); data_temps_stack.pop_back();
      data_temps[op.data_dest_index].bit_neg(*rd);
      data_temps_stack.push_back(&data_temps[op.data_dest_index]);
      break;

    case ExprOpcode::VALID_HEADER:
      bool_temps_stack.push_back(header_temps_stack.back()->is_valid());
      header_temps_stack.pop_back();
      break;

    default:
      assert(0 && "invalid operand");
      break;
    }
  }

  switch(expr_type) {
  case ExprType::EXPR_BOOL:
    *b_res = bool_temps_stack.back();
    break;
  case ExprType::EXPR_DATA:
    d_res->set(*(data_temps_stack.back()));
    break;
  }
}

bool Expression::eval_bool(
  const PHV &phv, const std::vector<Data> &locals
) const {
  bool result;
  eval_(phv, ExprType::EXPR_BOOL, locals, &result, nullptr);
  return result;
}

Data Expression::eval_arith(
  const PHV &phv, const std::vector<Data> &locals
) const {
  Data result_ptr;
  eval_(phv, ExprType::EXPR_DATA, locals, nullptr, &result_ptr);
  return result_ptr;
}

void Expression::eval_arith(
  const PHV &phv, Data *data, const std::vector<Data> &locals
) const {
  eval_(phv, ExprType::EXPR_DATA, locals, nullptr, data);
}

int Expression::assign_dest_registers() {
  int registers_cnt = 0;
  std::stack<int> new_registers;
  for(auto &op : ops) {
    switch(op.opcode) {
    case ExprOpcode::ADD:
    case ExprOpcode::SUB:
    case ExprOpcode::MOD:
    case ExprOpcode::MUL:
    case ExprOpcode::SHIFT_LEFT:
    case ExprOpcode::SHIFT_RIGHT:
    case ExprOpcode::BIT_AND:
    case ExprOpcode::BIT_OR:
    case ExprOpcode::BIT_XOR:
      registers_cnt -= new_registers.top();
      new_registers.pop();
      registers_cnt -= new_registers.top();
      new_registers.pop();

      op.data_dest_index = registers_cnt;

      new_registers.push(1);
      registers_cnt += 1;
      break;

    case ExprOpcode::BIT_NEG:
      registers_cnt -= new_registers.top();
      new_registers.pop();

      op.data_dest_index = registers_cnt;

      new_registers.push(1);
      registers_cnt += 1;
      break;

    case ExprOpcode::LOAD_CONST:
    case ExprOpcode::LOAD_LOCAL:
    case ExprOpcode::LOAD_FIELD:
      new_registers.push(0);
      break;
      
    default:
      break;
    }
  }
  return registers_cnt;
}

ArithExpression VLHeaderExpression::resolve(header_id_t header_id) {
  assert(expr.built);

  std::vector<Op> &ops = expr.ops;
  for(size_t i = 0; i < ops.size(); i++) {
    Op &op = ops[i];
    if(op.opcode == ExprOpcode::LOAD_LOCAL) {
      op.opcode = ExprOpcode::LOAD_FIELD;
      op.field.field_offset = op.local_offset;
      op.field.header = header_id;
    }
  }
  return expr;
}
