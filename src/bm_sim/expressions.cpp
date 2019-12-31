/* Copyright 2013-2019 Barefoot Networks, Inc.
 * Copyright 2019 VMware, Inc.
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
 * Antonin Bas
 *
 */

#include <bm/bm_sim/expressions.h>
#include <bm/bm_sim/stacks.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/stateful.h>

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
    {"load_header_stack", ExprOpcode::LOAD_HEADER_STACK},
    {"load_last_header_stack_field", ExprOpcode::LOAD_LAST_HEADER_STACK_FIELD},
    {"load_union", ExprOpcode::LOAD_UNION},
    {"load_union_stack", ExprOpcode::LOAD_UNION_STACK},
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
    {"==h", ExprOpcode::EQ_HEADER},
    {"!=h", ExprOpcode::NEQ_HEADER},
    {"==u", ExprOpcode::EQ_UNION},
    {"!=u", ExprOpcode::NEQ_UNION},
    {"==b", ExprOpcode::EQ_BOOL},
    {"!=b", ExprOpcode::NEQ_BOOL},
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
    {"valid_union", ExprOpcode::VALID_UNION},
    {"?", ExprOpcode::TERNARY_OP},
    {"two_comp_mod", ExprOpcode::TWO_COMP_MOD},
    {"usat_cast", ExprOpcode::USAT_CAST},
    {"sat_cast", ExprOpcode::SAT_CAST},
    {"d2b", ExprOpcode::DATA_TO_BOOL},
    {"b2d", ExprOpcode::BOOL_TO_DATA},
    // backward-compatibility
    // dereference_stack and dereference_header_stack are equivalent
    {"dereference_stack", ExprOpcode::DEREFERENCE_HEADER_STACK},
    {"dereference_header_stack", ExprOpcode::DEREFERENCE_HEADER_STACK},
    {"dereference_union_stack", ExprOpcode::DEREFERENCE_UNION_STACK},
    {"last_stack_index", ExprOpcode::LAST_STACK_INDEX},
    {"size_stack", ExprOpcode::SIZE_STACK},
    {"access_field", ExprOpcode::ACCESS_FIELD},
    {"access_union_header", ExprOpcode::ACCESS_UNION_HEADER},
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

/* static */ ExprOpcode
ExprOpcodesUtils::get_eq_opcode(ExprType expr_type) {
  switch (expr_type) {
    case ExprType::DATA:
      return ExprOpcode::EQ_DATA;
    case ExprType::HEADER:
      return ExprOpcode::EQ_HEADER;
    case ExprType::BOOL:
      return ExprOpcode::EQ_BOOL;
    case ExprType::UNION:
      return ExprOpcode::EQ_UNION;
    default:
      break;
  }
  assert(0);
  return ExprOpcode::EQ_DATA;
}

/* static */ ExprOpcode
ExprOpcodesUtils::get_neq_opcode(ExprType expr_type) {
  switch (expr_type) {
    case ExprType::DATA:
      return ExprOpcode::NEQ_DATA;
    case ExprType::HEADER:
      return ExprOpcode::NEQ_HEADER;
    case ExprType::BOOL:
      return ExprOpcode::NEQ_BOOL;
    case ExprType::UNION:
      return ExprOpcode::NEQ_UNION;
    default:
      break;
  }
  assert(0);
  return ExprOpcode::NEQ_DATA;
}

/* static */
ExprType
ExprOpcodesUtils::get_opcode_type(ExprOpcode opcode) {
  switch (opcode) {
    case ExprOpcode::LOAD_FIELD:
    case ExprOpcode::LOAD_CONST:
    case ExprOpcode::LOAD_LOCAL:
    case ExprOpcode::LOAD_REGISTER_REF:
    case ExprOpcode::LOAD_REGISTER_GEN:
    case ExprOpcode::LOAD_LAST_HEADER_STACK_FIELD:
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
    case ExprOpcode::BIT_NEG:
    case ExprOpcode::TWO_COMP_MOD:
    case ExprOpcode::USAT_CAST:
    case ExprOpcode::SAT_CAST:
    case ExprOpcode::BOOL_TO_DATA:
    case ExprOpcode::LAST_STACK_INDEX:
    case ExprOpcode::SIZE_STACK:
    case ExprOpcode::ACCESS_FIELD:
      return ExprType::DATA;
    case ExprOpcode::LOAD_BOOL:
    case ExprOpcode::EQ_DATA:
    case ExprOpcode::NEQ_DATA:
    case ExprOpcode::GT_DATA:
    case ExprOpcode::LT_DATA:
    case ExprOpcode::GET_DATA:
    case ExprOpcode::LET_DATA:
    case ExprOpcode::EQ_HEADER:
    case ExprOpcode::NEQ_HEADER:
    case ExprOpcode::EQ_UNION:
    case ExprOpcode::NEQ_UNION:
    case ExprOpcode::EQ_BOOL:
    case ExprOpcode::NEQ_BOOL:
    case ExprOpcode::AND:
    case ExprOpcode::OR:
    case ExprOpcode::NOT:
    case ExprOpcode::VALID_HEADER:
    case ExprOpcode::VALID_UNION:
    case ExprOpcode::DATA_TO_BOOL:
      return ExprType::BOOL;
    case ExprOpcode::LOAD_HEADER:
    case ExprOpcode::DEREFERENCE_HEADER_STACK:
    case ExprOpcode::ACCESS_UNION_HEADER:
      return ExprType::HEADER;
    case ExprOpcode::LOAD_HEADER_STACK:
      return ExprType::HEADER_STACK;
    case ExprOpcode::LOAD_UNION:
    case ExprOpcode::DEREFERENCE_UNION_STACK:
      return ExprType::UNION;
    case ExprOpcode::LOAD_UNION_STACK:
      return ExprType::UNION_STACK;
    case ExprOpcode::TERNARY_OP:
      return ExprType::UNKNOWN;
    case ExprOpcode::SKIP:
      break;
  }
  assert(0);
  return ExprType::UNKNOWN;
}


Expression::Expression() {
  // trick so that empty expressions can still be executed
  build();
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
Expression::push_back_load_header_stack(header_stack_id_t header_stack) {
  Op op;
  op.opcode = ExprOpcode::LOAD_HEADER_STACK;
  op.header_stack = header_stack;
  ops.push_back(op);
}

void
Expression::push_back_load_last_header_stack_field(
    header_stack_id_t header_stack, int field_offset) {
  Op op;
  op.opcode = ExprOpcode::LOAD_LAST_HEADER_STACK_FIELD;
  op.stack_field = {header_stack, field_offset};
  ops.push_back(op);
}

void
Expression::push_back_load_header_union(header_union_id_t header_union) {
  Op op;
  op.opcode = ExprOpcode::LOAD_UNION;
  op.header_union = header_union;
  ops.push_back(op);
}

void
Expression::push_back_load_header_union_stack(
    header_union_stack_id_t header_union_stack) {
  Op op;
  op.opcode = ExprOpcode::LOAD_UNION_STACK;
  op.header_union_stack = header_union_stack;
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
Expression::push_back_access_field(int field_offset) {
  Op op;
  op.opcode = ExprOpcode::ACCESS_FIELD;
  op.field_offset = field_offset;
  ops.push_back(op);
}

void
Expression::push_back_access_union_header(int header_offset) {
  Op op;
  op.opcode = ExprOpcode::ACCESS_UNION_HEADER;
  op.header_offset = header_offset;
  ops.push_back(op);
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

struct ExpressionTemps {
  ExpressionTemps()
      : data_temps_size(4), data_temps(data_temps_size) { }

  void prepare(int data_registers_cnt) {
    while (data_temps_size < data_registers_cnt) {
      data_temps.emplace_back();
      data_temps_size++;
    }

    bool_temps_stack.clear();
    data_temps_stack.clear();
    header_temps_stack.clear();
    stack_temps_stack.clear();
    union_temps_stack.clear();
  }

  void push_bool(bool b) {
    bool_temps_stack.push_back(b);
  }

  bool pop_bool() {
    auto r = bool_temps_stack.back();
    bool_temps_stack.pop_back();
    return r;
  }

  void push_data(const Data *data) {
    data_temps_stack.push_back(data);
  }

  const Data *pop_data() {
    auto *r = data_temps_stack.back();
    data_temps_stack.pop_back();
    return r;
  }

  void push_header(const Header *hdr) {
    header_temps_stack.push_back(hdr);
  }

  const Header *pop_header() {
    auto *r = header_temps_stack.back();
    header_temps_stack.pop_back();
    return r;
  }

  void push_stack(const StackIface *stack) {
    stack_temps_stack.push_back(stack);
  }

  const StackIface *pop_stack() {
    auto *r = stack_temps_stack.back();
    stack_temps_stack.pop_back();
    return r;
  }

  const HeaderStack *pop_header_stack() {
    return static_cast<const HeaderStack *>(pop_stack());
  }

  const HeaderUnionStack *pop_union_stack() {
    return static_cast<const HeaderUnionStack *>(pop_stack());
  }

  void push_union(const HeaderUnion *hdr_union) {
    union_temps_stack.push_back(hdr_union);
  }

  const HeaderUnion *pop_union() {
    auto *r = union_temps_stack.back();
    union_temps_stack.pop_back();
    return r;
  }

  static ExpressionTemps &get_instance() {
    // using a static thread-local variable to avoid allocation new memory every
    // time an expression needs to be evaluated. An alternative could be a
    // custom stack allocator.
    static thread_local ExpressionTemps instance;
    return instance;
  }

  int data_temps_size;
  std::vector<Data> data_temps;

  // Logically, I am using these as stacks but experiments showed that using
  // vectors directly was more efficient.
  std::vector<bool> bool_temps_stack;
  std::vector<const Data *> data_temps_stack;
  std::vector<const Header *> header_temps_stack;
  std::vector<const StackIface *> stack_temps_stack;
  std::vector<const HeaderUnion *> union_temps_stack;
};

void
Expression::eval_(const PHV &phv,
                  const std::vector<Data> &locals,
                  ExpressionTemps *temps) const {
  assert(built);

  temps->prepare(data_registers_cnt);

  auto &data_temps = temps->data_temps;

  bool lb, rb;
  const Data *ld, *rd;
  const Header *lh, *rh;
  const HeaderUnion *lu, *ru;
  const HeaderStack *hs;
  const HeaderUnionStack *hus;

  for (size_t i = 0; i < ops.size(); i++) {
    const auto &op = ops[i];
    switch (op.opcode) {
      case ExprOpcode::LOAD_FIELD:
        temps->push_data(
            &phv.get_field(op.field.header, op.field.field_offset));
        break;

      case ExprOpcode::LOAD_HEADER:
        temps->push_header(&phv.get_header(op.header));
        break;

      case ExprOpcode::LOAD_HEADER_STACK:
        temps->push_stack(&phv.get_header_stack(op.header_stack));
        break;

      case ExprOpcode::LOAD_LAST_HEADER_STACK_FIELD:
        temps->push_data(
            &phv.get_header_stack(op.stack_field.header_stack).get_last()
              .get_field(op.stack_field.field_offset));
        break;

      case ExprOpcode::LOAD_UNION:
        temps->push_union(&phv.get_header_union(op.header_union));
        break;

      case ExprOpcode::LOAD_UNION_STACK:
        temps->push_stack(&phv.get_header_union_stack(op.header_union_stack));
        break;

      case ExprOpcode::LOAD_BOOL:
        temps->push_bool(op.bool_value);
        break;

      case ExprOpcode::LOAD_CONST:
        temps->push_data(&const_values[op.const_offset]);
        break;

      case ExprOpcode::LOAD_LOCAL:
        temps->push_data(&locals[op.local_offset]);
        break;

      case ExprOpcode::LOAD_REGISTER_REF:
        temps->push_data(&op.register_ref.array->at(op.register_ref.idx));
        break;

      case ExprOpcode::LOAD_REGISTER_GEN:
        rd = temps->pop_data();
        temps->push_data(&op.register_array->at(rd->get<size_t>()));
        break;

      case ExprOpcode::ACCESS_FIELD:
        rh = temps->pop_header();
        temps->push_data(&rh->get_field(op.field_offset));
        break;

      case ExprOpcode::ACCESS_UNION_HEADER:
        ru = temps->pop_union();
        temps->push_header(&ru->at(op.header_offset));
        break;

      case ExprOpcode::ADD:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].add(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::SUB:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].sub(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::MOD:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].mod(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::DIV:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].divide(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::MUL:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].multiply(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::SHIFT_LEFT:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].shift_left(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::SHIFT_RIGHT:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].shift_right(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::EQ_DATA:
        rd = temps->pop_data();
        ld = temps->pop_data();
        temps->push_bool(*ld == *rd);
        break;

      case ExprOpcode::NEQ_DATA:
        rd = temps->pop_data();
        ld = temps->pop_data();
        temps->push_bool(*ld != *rd);
        break;

      case ExprOpcode::GT_DATA:
        rd = temps->pop_data();
        ld = temps->pop_data();
        temps->push_bool(*ld > *rd);
        break;

      case ExprOpcode::LT_DATA:
        rd = temps->pop_data();
        ld = temps->pop_data();
        temps->push_bool(*ld < *rd);
        break;

      case ExprOpcode::GET_DATA:
        rd = temps->pop_data();
        ld = temps->pop_data();
        temps->push_bool(*ld >= *rd);
        break;

      case ExprOpcode::LET_DATA:
        rd = temps->pop_data();
        ld = temps->pop_data();
        temps->push_bool(*ld <= *rd);
        break;

      case ExprOpcode::EQ_HEADER:
        rh = temps->pop_header();
        lh = temps->pop_header();
        temps->push_bool(lh->cmp(*rh));
        break;

      case ExprOpcode::NEQ_HEADER:
        rh = temps->pop_header();
        lh = temps->pop_header();
        temps->push_bool(!lh->cmp(*rh));
        break;

      case ExprOpcode::EQ_UNION:
        ru = temps->pop_union();
        lu = temps->pop_union();
        temps->push_bool(lu->cmp(*ru));
        break;

      case ExprOpcode::NEQ_UNION:
        ru = temps->pop_union();
        lu = temps->pop_union();
        temps->push_bool(!lu->cmp(*ru));
        break;

      case ExprOpcode::EQ_BOOL:
        rb = temps->pop_bool();
        lb = temps->pop_bool();
        temps->push_bool(lb == rb);
        break;

      case ExprOpcode::NEQ_BOOL:
        rb = temps->pop_bool();
        lb = temps->pop_bool();
        temps->push_bool(lb != rb);
        break;

      case ExprOpcode::AND:
        rb = temps->pop_bool();
        lb = temps->pop_bool();
        temps->push_bool(lb && rb);
        break;

      case ExprOpcode::OR:
        rb = temps->pop_bool();
        lb = temps->pop_bool();
        temps->push_bool(lb || rb);
        break;

      case ExprOpcode::NOT:
        rb = temps->pop_bool();
        temps->push_bool(!rb);
        break;

      case ExprOpcode::BIT_AND:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].bit_and(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::BIT_OR:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].bit_or(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::BIT_XOR:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].bit_xor(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::BIT_NEG:
        rd = temps->pop_data();
        data_temps[op.data_dest_index].bit_neg(*rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::VALID_HEADER:
        rh = temps->pop_header();
        temps->push_bool(rh->is_valid());
        break;

      case ExprOpcode::VALID_UNION:
        ru = temps->pop_union();
        temps->push_bool(ru->is_valid());
        break;

      case ExprOpcode::TERNARY_OP:
        rb = temps->pop_bool();
        if (rb) i += 1;
        break;

      case ExprOpcode::SKIP:
        i += op.skip_num;
        break;

      case ExprOpcode::TWO_COMP_MOD:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].two_comp_mod(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::USAT_CAST:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].usat_cast(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::SAT_CAST:
        rd = temps->pop_data();
        ld = temps->pop_data();
        data_temps[op.data_dest_index].sat_cast(*ld, *rd);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::DATA_TO_BOOL:
        rd = temps->pop_data();
        temps->push_bool(!rd->test_eq(0));
        break;

      case ExprOpcode::BOOL_TO_DATA:
        rb = temps->pop_bool();
        data_temps[op.data_dest_index].set(static_cast<int>(rb));
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::DEREFERENCE_HEADER_STACK:
        rd = temps->pop_data();
        hs = temps->pop_header_stack();
        temps->push_header(&hs->at(rd->get<size_t>()));
        break;

      // LAST_STACK_INDEX seems a little redundant given SIZE_STACK, but I don't
      // exclude in the future to do some sanity checking for LAST_STACK_INDEX
      case ExprOpcode::LAST_STACK_INDEX:
        hs = temps->pop_header_stack();
        data_temps[op.data_dest_index].set(hs->get_count() - 1);
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::SIZE_STACK:
        hs = temps->pop_header_stack();
        data_temps[op.data_dest_index].set(hs->get_count());
        temps->push_data(&data_temps[op.data_dest_index]);
        break;

      case ExprOpcode::DEREFERENCE_UNION_STACK:
        rd = temps->pop_data();
        hus = temps->pop_union_stack();
        temps->push_union(&hus->at(rd->get<size_t>()));
        break;

      default:
        assert(0 && "invalid operand");
        break;
    }
  }
}

bool
Expression::eval_bool(const PHV &phv, const std::vector<Data> &locals) const {
  // special case, where the expression is empty
  // not sure if this is the best way to handle this case, maybe the compiler
  // should make sure this never happens instead and we should treat this as
  // an error
  if (ops.empty()) return false;
  auto &temps = ExpressionTemps::get_instance();
  eval_(phv, locals, &temps);
  return temps.pop_bool();
}

Data
Expression::eval_arith(const PHV &phv, const std::vector<Data> &locals) const {
  if (ops.empty()) return Data(0);
  auto &temps = ExpressionTemps::get_instance();
  eval_(phv, locals, &temps);
  return *temps.pop_data();
}

void
Expression::eval_arith(const PHV &phv, Data *data,
                       const std::vector<Data> &locals) const {
  if (ops.empty()) {
    data->set(0);
    return;
  }
  auto &temps = ExpressionTemps::get_instance();
  eval_(phv, locals, &temps);
  data->set(*temps.pop_data());
}

// Unfortunately all the methods use a const_cast. I wanted to avoid having to
// change the interface for existing methods (eval_arith and eval_bool), for
// which the expectation is that the PHV will not be modified during
// evaluation. This meant that changing the private eval_ method was
// difficult. I haven't found a good solution, which doesn't make the code more
// complex. yet.

Data &
Expression::eval_arith_lvalue(PHV *phv, const std::vector<Data> &locals) const {
  assert(!ops.empty());
  auto &temps = ExpressionTemps::get_instance();
  eval_(*phv, locals, &temps);
  return const_cast<Data &>(*temps.pop_data());
}

Header &
Expression::eval_header(PHV *phv, const std::vector<Data> &locals) const {
  assert(!ops.empty());
  auto &temps = ExpressionTemps::get_instance();
  eval_(*phv, locals, &temps);
  return const_cast<Header &>(*temps.pop_header());
}

HeaderStack &
Expression::eval_header_stack(PHV *phv, const std::vector<Data> &locals) const {
  assert(!ops.empty());
  auto &temps = ExpressionTemps::get_instance();
  eval_(*phv, locals, &temps);
  return const_cast<HeaderStack &>(*temps.pop_header_stack());
}

HeaderUnion &
Expression::eval_header_union(PHV *phv, const std::vector<Data> &locals) const {
  assert(!ops.empty());
  auto &temps = ExpressionTemps::get_instance();
  eval_(*phv, locals, &temps);
  return const_cast<HeaderUnion &>(*temps.pop_union());
}

HeaderUnionStack &
Expression::eval_header_union_stack(
      PHV *phv, const std::vector<Data> &locals) const {
  assert(!ops.empty());
  auto &temps = ExpressionTemps::get_instance();
  eval_(*phv, locals, &temps);
  return const_cast<HeaderUnionStack &>(*temps.pop_union_stack());
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
      case ExprOpcode::USAT_CAST:
      case ExprOpcode::SAT_CAST:
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
      case ExprOpcode::LAST_STACK_INDEX:
      case ExprOpcode::SIZE_STACK:
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
      case ExprOpcode::DEREFERENCE_HEADER_STACK:
      case ExprOpcode::DEREFERENCE_UNION_STACK:
        registers_curr -= new_registers.top();
        new_registers.pop();
        break;

      case ExprOpcode::LOAD_CONST:
      case ExprOpcode::LOAD_LOCAL:
      case ExprOpcode::LOAD_FIELD:
      case ExprOpcode::LOAD_LAST_HEADER_STACK_FIELD:
      case ExprOpcode::LOAD_REGISTER_REF:
      case ExprOpcode::ACCESS_FIELD:
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

bool
Expression::empty() const {
  return ops.empty();
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
