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

#include <bm/bm_sim/expressions.h>
#include <bm/bm_sim/phv.h>

#include <stack>
#include <string>
#include <vector>
#include <algorithm>  // for std::max

#include <cassert>

namespace bm {

ExprOpcodesMap::ExprOpcodesMap() {
  opcodes_map = {
    {"load_field", ExprOpcode::LOAD_FIELD},
    {"load_header", ExprOpcode::LOAD_HEADER},
    {"load_bool", ExprOpcode::LOAD_BOOL},
    {"load_const", ExprOpcode::LOAD_CONST},
    {"load_local", ExprOpcode::LOAD_LOCAL},
    {"load_register_ref", ExprOpcode::LOAD_REGISTER_REF},
    {"load_register_gen", ExprOpcode::LOAD_REGISTER_GEN},
    {"+", ExprOpcode::ADD},
    {"-", ExprOpcode::SUB},
    {"%", ExprOpcode::MOD},
    {"/", ExprOpcode::DIV},
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
    {"?", ExprOpcode::TERNARY_OP},
    {"two_comp_mod", ExprOpcode::TWO_COMP_MOD},
    {"d2b", ExprOpcode::DATA_TO_BOOL},
    {"b2d", ExprOpcode::BOOL_TO_DATA},
  };
}

ExprOpcodesMap *ExprOpcodesMap::get_instance() {
  static ExprOpcodesMap instance;
  return &instance;
}

ExprOpcode
ExprOpcodesMap::get_opcode(std::string expr_name) {
  ExprOpcodesMap *instance = get_instance();
  return instance->opcodes_map[expr_name];
}

size_t
Expression::get_num_ops() const {
  return ops.size();
}

void
Expression::push_back_load_field(header_id_t header, int field_offset) {
  Op op;
  op.opcode = ExprOpcode::LOAD_FIELD;
  op.field = {header, field_offset};
  ops.push_back(op);
}

void
Expression::push_back_load_bool(bool value) {
  Op op;
  op.opcode = ExprOpcode::LOAD_BOOL;
  op.bool_value = value;
  ops.push_back(op);
}

void
Expression::push_back_load_header(header_id_t header) {
  Op op;
  op.opcode = ExprOpcode::LOAD_HEADER;
  op.header = header;
  ops.push_back(op);
}

void
Expression::push_back_load_const(const Data &data) {
  const_values.push_back(data);
  Op op;
  op.opcode = ExprOpcode::LOAD_CONST;
  op.const_offset = const_values.size() - 1;
  ops.push_back(op);
}

void
Expression::push_back_load_local(const int offset) {
  Op op;
  op.opcode = ExprOpcode::LOAD_LOCAL;
  op.local_offset = offset;
  ops.push_back(op);
}

void
Expression::push_back_load_register_ref(RegisterArray *register_array,
                                        unsigned int idx) {
  Op op;
  op.opcode = ExprOpcode::LOAD_REGISTER_REF;
  op.register_ref.array = register_array;
  op.register_ref.idx = idx;
  ops.push_back(op);
}

void
Expression::push_back_load_register_gen(RegisterArray *register_array) {
  Op op;
  op.opcode = ExprOpcode::LOAD_REGISTER_GEN;
  op.register_array = register_array;
  ops.push_back(op);
}

void
Expression::push_back_op(ExprOpcode opcode) {
  Op op;
  op.opcode = opcode;
  ops.push_back(op);
}

void
Expression::append_expression(const Expression &e) {
  int offset_consts = const_values.size();
  // the tricky part: update the const data offsets in the expression we are
  // appending
  for (auto &op : e.ops) {
    ops.push_back(op);
    if (op.opcode == ExprOpcode::LOAD_CONST)
      ops.back().const_offset += offset_consts;
  }
  const_values.insert(const_values.end(),
                      e.const_values.begin(), e.const_values.end());
}

// A note on the implementation of the ternary operator:
// The difficulty here is that the second and third expression are conditionally
// evaluated based on the result of the first expression (which evaluates to a
// boolean).
// I considered many different solutions, but in the end I decided to flatten
// the second and third expression ops into the main ops vector. For this, I had
// to introduce the special SKIP opcode. SKIP lets the action egine skip a
// pre-determined number of operations. For each ternary op, 2 SKIP ops are
// inserted, one before the second expression op sequence, and one before the
// third expression op sequence. When the condition evaluates to true, we leap
// over the first SKIP, execute all of the second expression ops, then reach the
// second SKIP which makes us skip all of the third expression ops. On the other
// hand, when the condition evaluates to false, we skip all of the second
// expression ops to go directly to the third expression ops.

void
Expression::push_back_ternary_op(const Expression &e1, const Expression &e2) {
  Op op;
  op.opcode = ExprOpcode::TERNARY_OP;
  ops.push_back(op);
  op.opcode = ExprOpcode::SKIP;
  op.skip_num = e1.get_num_ops() + 1;
  ops.push_back(op);
  append_expression(e1);
  op.skip_num = e2.get_num_ops();
  ops.push_back(op);
  append_expression(e2);
}

void
Expression::build() {
  data_registers_cnt = assign_dest_registers();
  built = true;
}

void
Expression::grab_register_accesses(RegisterSync *register_sync) const {
  for (auto &op : ops) {
    switch (op.opcode) {
      case ExprOpcode::LOAD_REGISTER_REF:
      case ExprOpcode::LOAD_REGISTER_GEN:
        register_sync->add_register_array(op.register_array);
        break;
      default:
        continue;
    }
  }
}

/* I have made this function more efficient by using thread_local variables
   instead of dynamic allocation at each call. Maybe it would be better to just
   try to use a stack allocator */
void
Expression::eval_(const PHV &phv, ExprType expr_type,
                  const std::vector<Data> &locals,
                  bool *b_res, Data *d_res) const {
  assert(built);

  static thread_local int data_temps_size = 4;
  // std::vector<Data> data_temps(data_registers_cnt);
  static thread_local std::vector<Data> data_temps(data_temps_size);
  while (data_temps_size < data_registers_cnt) {
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

  for (size_t i = 0; i < ops.size(); i++) {
    const auto &op = ops[i];
    switch (op.opcode) {
      case ExprOpcode::LOAD_FIELD:
        data_temps_stack.push_back(
            &(phv.get_field(op.field.header, op.field.field_offset)));
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

      case ExprOpcode::LOAD_REGISTER_REF:
        data_temps_stack.push_back(
            &op.register_ref.array->at(op.register_ref.idx));
        break;

      case ExprOpcode::LOAD_REGISTER_GEN:
        rd = data_temps_stack.back(); data_temps_stack.pop_back();
        data_temps_stack.push_back(&op.register_array->at(rd->get<size_t>()));
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

      case ExprOpcode::MOD:
        rd = data_temps_stack.back(); data_temps_stack.pop_back();
        ld = data_temps_stack.back(); data_temps_stack.pop_back();
        data_temps[op.data_dest_index].mod(*ld, *rd);
        data_temps_stack.push_back(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::DIV:
        rd = data_temps_stack.back(); data_temps_stack.pop_back();
        ld = data_temps_stack.back(); data_temps_stack.pop_back();
        data_temps[op.data_dest_index].divide(*ld, *rd);
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

      case ExprOpcode::TERNARY_OP:
        if (bool_temps_stack.back())
          i += 1;
        bool_temps_stack.pop_back();
        break;

      case ExprOpcode::SKIP:
        i += op.skip_num;
        break;

      case ExprOpcode::TWO_COMP_MOD:
        rd = data_temps_stack.back(); data_temps_stack.pop_back();
        ld = data_temps_stack.back(); data_temps_stack.pop_back();
        data_temps[op.data_dest_index].two_comp_mod(*ld, *rd);
        data_temps_stack.push_back(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::DATA_TO_BOOL:
        rd = data_temps_stack.back(); data_temps_stack.pop_back();
        bool_temps_stack.push_back(!rd->test_eq(0));
        break;

      case ExprOpcode::BOOL_TO_DATA:
        rb = bool_temps_stack.back(); bool_temps_stack.pop_back();
        data_temps[op.data_dest_index].set(static_cast<int>(rb));
        data_temps_stack.push_back(&data_temps[op.data_dest_index]);
        break;

      default:
        assert(0 && "invalid operand");
        break;
    }
  }

  switch (expr_type) {
    case ExprType::EXPR_BOOL:
      *b_res = bool_temps_stack.back();
      break;
    case ExprType::EXPR_DATA:
      d_res->set(*(data_temps_stack.back()));
      break;
  }
}

bool
Expression::eval_bool(const PHV &phv, const std::vector<Data> &locals) const {
  bool result;
  eval_(phv, ExprType::EXPR_BOOL, locals, &result, nullptr);
  return result;
}

Data
Expression::eval_arith(const PHV &phv, const std::vector<Data> &locals) const {
  Data result_ptr;
  eval_(phv, ExprType::EXPR_DATA, locals, nullptr, &result_ptr);
  return result_ptr;
}

void
Expression::eval_arith(const PHV &phv, Data *data,
                       const std::vector<Data> &locals) const {
  eval_(phv, ExprType::EXPR_DATA, locals, nullptr, data);
}

// TODO(antonin): If there is a ternary op, we will over-estimate this number,
// see if there is an easy fix
int
Expression::assign_dest_registers() {
  int registers_cnt = 0;
  int registers_curr = 0;
  std::stack<int> new_registers;
  for (auto &op : ops) {
    switch (op.opcode) {
      case ExprOpcode::ADD:
      case ExprOpcode::SUB:
      case ExprOpcode::MOD:
      case ExprOpcode::DIV:
      case ExprOpcode::MUL:
      case ExprOpcode::SHIFT_LEFT:
      case ExprOpcode::SHIFT_RIGHT:
      case ExprOpcode::BIT_AND:
      case ExprOpcode::BIT_OR:
      case ExprOpcode::BIT_XOR:
      case ExprOpcode::TWO_COMP_MOD:
        registers_curr -= new_registers.top();
        new_registers.pop();
        registers_curr -= new_registers.top();
        new_registers.pop();

        op.data_dest_index = registers_curr;

        new_registers.push(1);
        registers_curr += 1;
        break;

      case ExprOpcode::BIT_NEG:
        registers_curr -= new_registers.top();
        new_registers.pop();

        op.data_dest_index = registers_curr;

        new_registers.push(1);
        registers_curr += 1;
        break;

      case ExprOpcode::BOOL_TO_DATA:
        op.data_dest_index = registers_curr;

        new_registers.push(1);
        registers_curr += 1;
        break;

      // added recently; not necessary but could decrease number of registers
      case ExprOpcode::EQ_DATA:
      case ExprOpcode::NEQ_DATA:
      case ExprOpcode::GT_DATA:
      case ExprOpcode::LT_DATA:
      case ExprOpcode::GET_DATA:
      case ExprOpcode::LET_DATA:
        registers_curr -= new_registers.top();
        new_registers.pop();
        registers_curr -= new_registers.top();
        new_registers.pop();
        break;

      case ExprOpcode::DATA_TO_BOOL:
        registers_curr -= new_registers.top();
        new_registers.pop();
        break;

      case ExprOpcode::LOAD_CONST:
      case ExprOpcode::LOAD_LOCAL:
      case ExprOpcode::LOAD_FIELD:
      case ExprOpcode::LOAD_REGISTER_REF:
        new_registers.push(0);
        break;

      case ExprOpcode::LOAD_REGISTER_GEN:
        registers_curr -= new_registers.top();
        new_registers.pop();

        new_registers.push(0);
        break;

      // here to emphasize the fact that with my skip implementation choice,
      // nothing special needs to be done here
      case ExprOpcode::TERNARY_OP:
        break;

      default:
        break;
    }

    registers_cnt = std::max(registers_cnt, registers_curr);
  }

  return registers_cnt;
}

VLHeaderExpression::VLHeaderExpression(const ArithExpression &expr)
    : expr(expr) {
  for (const Op &op : expr.ops) {
    if (op.opcode == ExprOpcode::LOAD_LOCAL) {
      offsets.push_back(op.local_offset);
    }
  }
}

const std::vector<int> &
VLHeaderExpression::get_input_offsets() const {
  return offsets;
}

ArithExpression
VLHeaderExpression::resolve(header_id_t header_id) {
  assert(expr.built);

  ArithExpression new_expr = expr;

  std::vector<Op> &ops = new_expr.ops;
  for (size_t i = 0; i < ops.size(); i++) {
    Op &op = ops[i];
    if (op.opcode == ExprOpcode::LOAD_LOCAL) {
      op.opcode = ExprOpcode::LOAD_FIELD;
      op.field.field_offset = op.local_offset;
      op.field.header = header_id;
    }
  }
  return new_expr;
}

}  // namespace bm
