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

#ifndef BM_BM_SIM_EXPRESSIONS_H_
#define BM_BM_SIM_EXPRESSIONS_H_

#include <unordered_map>
#include <string>
#include <vector>

#include "data.h"
#include "phv_forward.h"
#include "stateful.h"

namespace bm {

enum class ExprOpcode {
  LOAD_FIELD, LOAD_HEADER, LOAD_BOOL, LOAD_CONST, LOAD_LOCAL,
  LOAD_REGISTER_REF, LOAD_REGISTER_GEN,
  ADD, SUB, MOD, DIV, MUL, SHIFT_LEFT, SHIFT_RIGHT,
  EQ_DATA, NEQ_DATA, GT_DATA, LT_DATA, GET_DATA, LET_DATA,
  AND, OR, NOT,
  BIT_AND, BIT_OR, BIT_XOR, BIT_NEG,
  VALID_HEADER,
  TERNARY_OP, SKIP,
  TWO_COMP_MOD,
  DATA_TO_BOOL, BOOL_TO_DATA
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

    int local_offset;

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
  Expression() { }

  void push_back_load_field(header_id_t header, int field_offset);
  void push_back_load_bool(bool value);
  void push_back_load_header(header_id_t header);
  void push_back_load_const(const Data &data);
  void push_back_load_local(const int offset);
  void push_back_load_register_ref(RegisterArray *register_array,
                                   unsigned int idx);
  void push_back_load_register_gen(RegisterArray *register_array);
  void push_back_op(ExprOpcode opcode);
  void push_back_ternary_op(const Expression &e1, const Expression &e2);

  void build();

  void grab_register_accesses(RegisterSync *register_sync) const;

  bool eval_bool(const PHV &phv, const std::vector<Data> &locals = {}) const;
  Data eval_arith(const PHV &phv, const std::vector<Data> &locals = {}) const;
  void eval_arith(const PHV &phv, Data *data,
                  const std::vector<Data> &locals = {}) const;

  // I am authorizing copy for this object
  Expression(const Expression &other) = default;
  Expression &operator=(const Expression &other) = default;

  Expression(Expression &&other) /*noexcept*/ = default;
  Expression &operator=(Expression &&other) /*noexcept*/ = default;

 private:
  enum class ExprType {EXPR_BOOL, EXPR_DATA};

 private:
  int assign_dest_registers();
  void eval_(const PHV &phv, ExprType expr_type,
             const std::vector<Data> &locals,
             bool *b_res, Data *d_res) const;
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
