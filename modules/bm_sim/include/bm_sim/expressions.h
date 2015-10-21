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

#ifndef _BM_EXPRESSIONS_H_
#define _BM_EXPRESSIONS_H_

#include <unordered_map>
#include <string>
#include <vector>

#include "data.h"
#include "phv_forward.h"

enum class ExprOpcode {
  LOAD_FIELD, LOAD_HEADER, LOAD_BOOL, LOAD_CONST, LOAD_LOCAL,
  ADD, SUB, MOD, MUL, SHIFT_LEFT, SHIFT_RIGHT,
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

    int local_offset;
  };
};

class Expression
{
public:
  Expression() { }

  void push_back_load_field(header_id_t header, int field_offset);
  void push_back_load_bool(bool value);
  void push_back_load_header(header_id_t header);
  void push_back_load_const(const Data &data);
  void push_back_load_local(const int offset);
  void push_back_op(ExprOpcode opcode);

  void build();

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
  
private:
  std::vector<Op> ops{};
  std::vector<Data> const_values{};
  int data_registers_cnt{0};
  bool built{false};

  friend class VLHeaderExpression;
};


class ArithExpression : public Expression
{
public:
  void eval(const PHV &phv, Data *data,
	    const std::vector<Data> &locals = {}) const {
    eval_arith(phv, data, locals);
  }
};

class VLHeaderExpression
{
public:
  VLHeaderExpression(const ArithExpression &expr)
    : expr(expr) {}

  ArithExpression resolve(header_id_t header_id);

private:
  ArithExpression expr;
};

#endif
