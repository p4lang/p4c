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

#ifndef BM_BM_SIM_EXPRESSIONS_H_
#define BM_BM_SIM_EXPRESSIONS_H_

#include <unordered_map>
#include <string>
#include <vector>

#include "data.h"
#include "headers.h"
#include "header_unions.h"
#include "phv_forward.h"
#include "stacks.h"

namespace bm {

class RegisterArray;
class RegisterSync;

struct ExpressionTemps;

enum class ExprOpcode {
  LOAD_FIELD, LOAD_HEADER, LOAD_HEADER_STACK, LOAD_LAST_HEADER_STACK_FIELD,
  LOAD_UNION, LOAD_UNION_STACK, LOAD_BOOL, LOAD_CONST, LOAD_LOCAL,
  LOAD_REGISTER_REF, LOAD_REGISTER_GEN,
  ADD, SUB, MOD, DIV, MUL, SHIFT_LEFT, SHIFT_RIGHT,
  EQ_DATA, NEQ_DATA, GT_DATA, LT_DATA, GET_DATA, LET_DATA,
  EQ_HEADER, NEQ_HEADER,
  EQ_UNION, NEQ_UNION,
  EQ_BOOL, NEQ_BOOL,
  AND, OR, NOT,
  BIT_AND, BIT_OR, BIT_XOR, BIT_NEG,
  VALID_HEADER, VALID_UNION,
  TERNARY_OP, SKIP,
  TWO_COMP_MOD,
  USAT_CAST, SAT_CAST,
  DATA_TO_BOOL, BOOL_TO_DATA,
  DEREFERENCE_HEADER_STACK,
  DEREFERENCE_UNION_STACK,
  LAST_STACK_INDEX, SIZE_STACK,
  ACCESS_FIELD,
  ACCESS_UNION_HEADER,
};

enum class ExprType {
  UNKNOWN, DATA, HEADER, HEADER_STACK, BOOL, UNION, UNION_STACK
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

class ExprOpcodesUtils {
 public:
  static ExprOpcode get_eq_opcode(ExprType expr_type);
  static ExprOpcode get_neq_opcode(ExprType expr_type);

  static ExprType get_opcode_type(ExprOpcode opcode);
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

    header_stack_id_t header_stack;

    struct {
      header_stack_id_t header_stack;
      int field_offset;
    } stack_field;

    header_union_id_t header_union;

    header_union_stack_id_t header_union_stack;

    bool bool_value;

    int const_offset;

    int local_offset;

    int field_offset;

    int header_offset;

    // In theory, if registers cannot be resized, I could directly store a
    // pointer to the correct register cell, i.e. &(*array)[idx]. However, this
    // gives me more flexibility in case I want to be able to resize the
    // registers arbitrarily in the future.
    struct {
      RegisterArray *array;
      unsigned int idx;
    } register_ref;

    RegisterArray *register_array;

    int skip_num;
  };
};

class Expression {
 public:
  Expression();

  virtual ~Expression() { }

  void push_back_load_field(header_id_t header, int field_offset);
  void push_back_load_bool(bool value);
  void push_back_load_header(header_id_t header);
  void push_back_load_header_stack(header_stack_id_t header_stack);
  void push_back_load_last_header_stack_field(header_stack_id_t header_stack,
                                              int field_offset);
  void push_back_load_header_union(header_union_id_t header_union);
  void push_back_load_header_union_stack(
      header_union_stack_id_t header_union_stack);
  void push_back_load_const(const Data &data);
  void push_back_load_local(const int offset);
  void push_back_load_register_ref(RegisterArray *register_array,
                                   unsigned int idx);
  void push_back_load_register_gen(RegisterArray *register_array);
  void push_back_op(ExprOpcode opcode);
  void push_back_ternary_op(const Expression &e1, const Expression &e2);
  void push_back_access_field(int field_offset);
  void push_back_access_union_header(int header_offset);

  void build();

  void grab_register_accesses(RegisterSync *register_sync) const;

  bool eval_bool(const PHV &phv, const std::vector<Data> &locals = {}) const;
  Data eval_arith(const PHV &phv, const std::vector<Data> &locals = {}) const;
  void eval_arith(const PHV &phv, Data *data,
                  const std::vector<Data> &locals = {}) const;
  Data &eval_arith_lvalue(PHV *phv, const std::vector<Data> &locals = {}) const;
  Header &eval_header(PHV *phv, const std::vector<Data> &locals = {}) const;
  HeaderStack &eval_header_stack(
      PHV *phv, const std::vector<Data> &locals = {}) const;
  HeaderUnion &eval_header_union(
      PHV *phv, const std::vector<Data> &locals = {}) const;
  HeaderUnionStack &eval_header_union_stack(
      PHV *phv, const std::vector<Data> &locals = {}) const;

  bool empty() const;

  // I am authorizing copy for this object
  Expression(const Expression &other) = default;
  Expression &operator=(const Expression &other) = default;

  Expression(Expression &&other) /*noexcept*/ = default;
  Expression &operator=(Expression &&other) /*noexcept*/ = default;

 private:
  int assign_dest_registers();
  void eval_(const PHV &phv,
             const std::vector<Data> &locals,
             ExpressionTemps *temps) const;
  size_t get_num_ops() const;
  void append_expression(const Expression &e);

 private:
  std::vector<Op> ops{};
  std::vector<Data> const_values{};
  int data_registers_cnt{0};
  bool built{false};

  friend class VLHeaderExpression;
};


class ArithExpression : public Expression {
 public:
  void eval(const PHV &phv, Data *data,
            const std::vector<Data> &locals = {}) const {
    eval_arith(phv, data, locals);
  }

  Data &eval_lvalue(PHV *phv, const std::vector<Data> &locals = {}) const {
    return eval_arith_lvalue(phv, locals);
  }
};


class BoolExpression : public Expression {
 public:
  bool eval(const PHV &phv, const std::vector<Data> &locals = {}) const {
    return eval_bool(phv, locals);
  }
};


class VLHeaderExpression {
 public:
  explicit VLHeaderExpression(const ArithExpression &expr);

  ArithExpression resolve(header_id_t header_id);

  const std::vector<int> &get_input_offsets() const;

 private:
  ArithExpression expr;
  std::vector<int> offsets{};
};

}  // namespace bm

#endif  // BM_BM_SIM_EXPRESSIONS_H_
