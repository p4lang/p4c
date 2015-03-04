#ifndef _BM_CONDITIONALS_H_
#define _BM_CONDITIONALS_H_

#include <unordered_map>
#include <string>
#include <vector>

#include "data.h"
#include "phv.h"
#include "control_flow.h"

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
  std::unordered_map<std::string, ExprOpcode> opcodes_map;
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

class Conditional : public ControlFlowNode {
public:
  Conditional() {}

  Conditional(const std::string &name)
    : name(name) {}

  void push_back_load_field(header_id_t header, int field_offset);
  void push_back_load_bool(bool value);
  void push_back_load_header(header_id_t header);
  void push_back_load_const(const Data &data);
  void push_back_op(ExprOpcode opcode);

  void build();

  bool eval(const PHV &phv);

  void set_next_node_if_true(ControlFlowNode *next_node) {
    true_next = next_node;
  }

  void set_next_node_if_false(ControlFlowNode *next_node) {
    false_next = next_node;
  }

  // return pointer to next control flow node
  ControlFlowNode *operator()(const Packet &pkt, PHV *phv) override;

private:
  int assign_dest_registers();
  
private:
  std::string name;
  std::vector<Op> ops;
  std::vector<Data> const_values;
  ControlFlowNode *true_next{nullptr};
  ControlFlowNode *false_next{nullptr};
  int data_registers_cnt{0};
  bool built{false};
};

#endif
