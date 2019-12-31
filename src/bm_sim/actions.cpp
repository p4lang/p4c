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

#include <bm/bm_sim/actions.h>
#include <bm/bm_sim/debugger.h>
#include <bm/bm_sim/event_logger.h>
#include <bm/bm_sim/P4Objects.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/logger.h>

#include <string>
#include <vector>

#include "utils.h"

namespace bm {

ActionEngineState::ActionEngineState(Packet *pkt,
                            const ActionData &action_data,
                            const std::vector<Data> &const_values,
                            const std::vector<ActionParam> &parameters_vector)
    : pkt(*pkt), phv(*pkt->get_phv()),
      action_data(action_data), const_values(const_values),
      parameters_vector(parameters_vector) { }

// the first tmp data register is reserved for internal engine use (register
// index evaluation)
size_t ActionFn::nb_data_tmps = 1;

ActionFn::ActionFn(const std::string &name, p4object_id_t id, size_t num_params)
    : NamedP4Object(name, id), num_params(num_params) { }

ActionFn::ActionFn(const std::string &name, p4object_id_t id, size_t num_params,
                   std::unique_ptr<SourceInfo> source_info)
    : NamedP4Object(name, id, std::move(source_info)),
      num_params(num_params) { }

void
ActionFn::parameter_push_back_field(header_id_t header, int field_offset) {
  ActionParam param;
  param.tag = ActionParam::FIELD;
  param.field = {header, field_offset};
  params.push_back(param);
}

void
ActionFn::parameter_push_back_header(header_id_t header) {
  ActionParam param;
  param.tag = ActionParam::HEADER;
  param.header = header;
  params.push_back(param);
}

void
ActionFn::parameter_push_back_header_stack(header_stack_id_t header_stack) {
  ActionParam param;
  param.tag = ActionParam::HEADER_STACK;
  param.header_stack = header_stack;
  params.push_back(param);
}

void
ActionFn::parameter_push_back_last_header_stack_field(
    header_stack_id_t header_stack, int field_offset) {
  ActionParam param;
  param.tag = ActionParam::LAST_HEADER_STACK_FIELD;
  param.stack_field = {header_stack, field_offset};
  params.push_back(param);
}

void
ActionFn::parameter_push_back_header_union(header_union_id_t header_union) {
  ActionParam param;
  param.tag = ActionParam::HEADER_UNION;
  param.header_union = header_union;
  params.push_back(param);
}

void
ActionFn::parameter_push_back_header_union_stack(
    header_union_stack_id_t header_union_stack) {
  ActionParam param;
  param.tag = ActionParam::HEADER_UNION_STACK;
  param.header_union_stack = header_union_stack;
  params.push_back(param);
}

void
ActionFn::parameter_push_back_const(const Data &data) {
  const_values.push_back(data);
  ActionParam param;
  param.tag = ActionParam::CONST;
  param.const_offset = const_values.size() - 1;;
  params.push_back(param);
}

void
ActionFn::parameter_push_back_action_data(int action_data_offset) {
  ActionParam param;
  param.tag = ActionParam::ACTION_DATA;
  param.action_data_offset = action_data_offset;
  params.push_back(param);
}

void
ActionFn::parameter_push_back_register_ref(RegisterArray *register_array,
                                           unsigned int idx) {
  ActionParam param;
  param.tag = ActionParam::REGISTER_REF;
  param.register_ref.array = register_array;
  param.register_ref.idx = idx;
  params.push_back(param);

  register_sync.add_register_array(register_array);
}

void
ActionFn::parameter_push_back_register_gen(
    RegisterArray *register_array, std::unique_ptr<ArithExpression> idx) {
  ActionParam param;
  param.tag = ActionParam::REGISTER_GEN;
  param.register_gen.array = register_array;

  idx->grab_register_accesses(&register_sync);

  param.register_gen.idx = idx.get();
  params.push_back(param);

  // does not invalidate param.register_gen.idx
  expressions.push_back(std::move(idx));

  register_sync.add_register_array(register_array);
}

void
ActionFn::parameter_push_back_calculation(const NamedCalculation *calculation) {
  ActionParam param;
  param.tag = ActionParam::CALCULATION;
  param.calculation = calculation;
  params.push_back(param);
}

void
ActionFn::parameter_push_back_meter_array(MeterArray *meter_array) {
  ActionParam param;
  param.tag = ActionParam::METER_ARRAY;
  param.meter_array = meter_array;
  params.push_back(param);
}

void
ActionFn::parameter_push_back_counter_array(CounterArray *counter_array) {
  ActionParam param;
  param.tag = ActionParam::COUNTER_ARRAY;
  param.counter_array = counter_array;
  params.push_back(param);
}

void
ActionFn::parameter_push_back_register_array(RegisterArray *register_array) {
  ActionParam param;
  param.tag = ActionParam::REGISTER_ARRAY;
  param.register_array = register_array;
  params.push_back(param);

  register_sync.add_register_array(register_array);
}

void
ActionFn::parameter_push_back_expression(std::unique_ptr<Expression> expr) {
  size_t nb_expression_params = 1;
  for (const auto &p : params)
    if (p.tag == ActionParam::EXPRESSION) nb_expression_params += 1;

  assert(nb_expression_params <= ActionFn::nb_data_tmps);
  if (nb_expression_params == ActionFn::nb_data_tmps)
    ActionFn::nb_data_tmps += 1;

  expr->grab_register_accesses(&register_sync);

  expressions.push_back(std::move(expr));
  ActionParam param;
  param.tag = ActionParam::EXPRESSION;
  param.expression = {static_cast<unsigned int>(nb_expression_params),
                      expressions.back().get()};
  params.push_back(param);
}

void
ActionFn::parameter_push_back_expression(std::unique_ptr<Expression> expr,
                                         ExprType expr_type) {
  if (expr_type == ExprType::DATA) {
    parameter_push_back_expression(std::move(expr));
    return;
  }

  expressions.push_back(std::move(expr));
  ActionParam param;
  switch (expr_type) {
    case ExprType::HEADER:
      param.tag = ActionParam::EXPRESSION_HEADER;
      break;
    case ExprType::HEADER_STACK:
      param.tag = ActionParam::EXPRESSION_HEADER_STACK;
      break;
    case ExprType::UNION:
      param.tag = ActionParam::EXPRESSION_HEADER_UNION;
      break;
    case ExprType::UNION_STACK:
      param.tag = ActionParam::EXPRESSION_HEADER_UNION_STACK;
      break;
    default:
      assert(0 && "Invalid expression type");
      return;
  }
  param.expression = {static_cast<unsigned int>(-1), expressions.back().get()};
  params.push_back(param);
}

void
ActionFn::parameter_push_back_extern_instance(ExternType *extern_instance) {
  ActionParam param;
  param.tag = ActionParam::EXTERN_INSTANCE;
  param.extern_instance = extern_instance;
  params.push_back(param);
}

void
ActionFn::parameter_push_back_string(const std::string &str) {
  strings.push_back(str);
  size_t idx = 0;
  // we called push_back on the vector which may have invalidated the pointers,
  // so we need to recompute them; alternatively we could add a level of
  // indirection and store std::unique_ptr<std::string> in strings...
  for (auto &p : params)
    if (p.tag == ActionParam::STRING) p.str = &strings.at(idx++);
  assert(strings.size() - 1 == idx);
  ActionParam param;
  param.tag = ActionParam::STRING;
  param.str = &strings.back();
  params.push_back(param);
}

void
ActionFn::parameter_start_vector() {
  ActionParam param;
  param.tag = ActionParam::PARAMS_VECTOR;
  auto start = static_cast<unsigned int>(sub_params.size());
  // end will be adjusted correctly when parameter_end_vector is called
  param.params_vector = {start, start /* end */};
  params.push_back(param);

  // we swap the 2 vectors so that subsequent calls to parameter_push_back_*
  // methods append parameters to the end of sub_params (instead of params)
  params.swap(sub_params);
}

void
ActionFn::parameter_end_vector() {
  params.swap(sub_params);
  assert(params.back().tag == ActionParam::PARAMS_VECTOR &&
         "no vector was started");
  auto end = static_cast<unsigned int>(sub_params.size());
  params.back().params_vector.end = end;
}

void
ActionFn::push_back_primitive(ActionPrimitive_ *primitive,
                              std::unique_ptr<SourceInfo> source_info) {
  size_t param_offset = 0;
  if (!primitives.empty()) {
    param_offset = primitives.back().get_param_offset();
    param_offset += primitives.back().get_num_params();
  }
  primitives.emplace_back(primitive, param_offset, std::move(source_info));
}

void
ActionFn::grab_register_accesses(RegisterSync *rs) const {
  rs->merge_from(register_sync);
}

size_t
ActionFn::get_num_params() const {
  return num_params;
}

namespace core {

extern int _bm_core_primitives_import();

}  // namespace core

ActionOpcodesMap::ActionOpcodesMap() {
  // ensures that core primitives are registered properly
  core::_bm_core_primitives_import();
}

bool
ActionOpcodesMap::register_primitive(
    const char *name,
    ActionPrimitiveFactoryFn fn) {
  const std::string str_name = std::string(name);
  auto it = map_.find(str_name);
  if (it != map_.end()) return false;
  map_[str_name] = std::move(fn);
  return true;
}

std::unique_ptr<ActionPrimitive_>
ActionOpcodesMap::get_primitive(const std::string &name) {
  auto it = map_.find(name);
  if (it == map_.end()) return nullptr;
  return it->second();
}

ActionOpcodesMap *
ActionOpcodesMap::get_instance() {
  static ActionOpcodesMap instance;
  return &instance;
}

ActionPrimitiveCall::ActionPrimitiveCall(
    ActionPrimitive_ *primitive, size_t param_offset,
    std::unique_ptr<SourceInfo> source_info)
    : primitive(primitive), param_offset(param_offset),
      source_info(std::move(source_info)) { }

void
ActionPrimitiveCall::execute(ActionEngineState *state,
                             const ActionParam *args) const {
  // TODO(unknown): log source info?
  primitive->set_source_info(source_info.get());
  primitive->execute(state, args);
}

size_t
ActionPrimitiveCall::get_num_params() const {
  return primitive->get_num_params();
}

void
ActionFnEntry::push_back_action_data(const Data &data) {
  action_data.push_back_action_data(data);
}

void
ActionFnEntry::push_back_action_data(unsigned int data) {
  action_data.push_back_action_data(data);
}

void
ActionFnEntry::push_back_action_data(const char *bytes, int nbytes) {
  action_data.push_back_action_data(bytes, nbytes);
}

void
ActionFnEntry::execute(Packet *pkt) const {
  ActionEngineState state(pkt, action_data, action_fn->const_values,
                          action_fn->sub_params);

  auto &primitives = action_fn->primitives;
  size_t param_offset = 0;
  BMLOG_TRACE_SI_PKT(*pkt, action_fn->get_source_info(),
                     "Action {}", action_fn->get_name());
  for (size_t idx = 0; idx < primitives.size();) {
    const auto &primitive = primitives[idx];
    BMLOG_TRACE_SI_PKT(*pkt, primitive.get_source_info(),
      "Primitive {}",
        (primitive.get_source_info() == nullptr) ? "(no source info)"
        : primitive.get_source_info()->get_source_fragment());
    param_offset = primitive.get_param_offset();
    primitive.execute(&state, &(action_fn->params[param_offset]));
    idx = primitive.get_jump_offset(idx);
  }
}

void
ActionFnEntry::operator()(Packet *pkt) const {
  if (!action_fn) return;  // empty action
  BMELOG(action_execute, *pkt, *action_fn, action_data);
  // TODO(antonin)
  // this is temporary while we experiment with the debugger
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_ACTION | action_fn->get_id());

  {
    RegisterSync::RegisterLocks RL;
    action_fn->register_sync.lock(&RL);
    execute(pkt);
  }

  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_EXIT(DBG_CTR_ACTION) | action_fn->get_id());
}

void
ActionFnEntry::dump(std::ostream *stream) const {
  if (action_fn == nullptr) {
    (*stream) << "NULL";
    return;
  }
  (*stream) << action_fn->name << " - ";
  // TODO(antonin): use utils::StreamStateSaver?
  std::ios_base::fmtflags ff;
  ff = std::cout.flags();
  for (const Data &d : action_data.action_data)
    (*stream) << std::hex << d << ",";
  stream->flags(ff);
}

void
ActionFnEntry::serialize(std::ostream *out) const {
  if (action_fn == nullptr) {
    (*out) << -1 << "\n";
    return;
  }
  (*out) << action_fn->id << "\n";
  (*out) << action_data.size() << "\n";
  utils::StreamStateSaver state_saver(*out);
  for (const Data &d : action_data.action_data)
    (*out) << std::hex << d << "\n";
}

void
ActionFnEntry::deserialize(std::istream *in, const P4Objects &objs) {
  p4object_id_t id; (*in) >> id;
  if (id == -1) {
    action_fn = nullptr;
    return;
  }
  action_fn = objs.get_action_by_id(id);
  assert(action_fn);
  size_t s; (*in) >> s;
  for (size_t i = 0; i < s; i++) {
    std::string data_hex; (*in) >> data_hex;
    push_back_action_data(Data(data_hex));
  }
}

thread_local Packet *ActionPrimitive_::pkt = nullptr;
thread_local PHV *ActionPrimitive_::phv = nullptr;

// TODO(antonin): should this be moved to core/ ?
namespace core {

struct _jump_if_zero : public ActionPrimitive<const Data &, const Data &> {
  void operator ()(const Data &test_v, const Data &jump_offset) override {
    if (test_v.get<int>() == 0)
      offset = jump_offset.get<int>();
    else
      offset = -1;
  }

  size_t get_jump_offset(size_t current_offset) const override {
    return (offset < 0) ? (current_offset + 1) : static_cast<size_t>(offset);
  }

 private:
  static thread_local int offset;
};

thread_local int _jump_if_zero::offset = -1;

struct _jump : public ActionPrimitive<const Data &> {
  void operator ()(const Data &jump_offset) override {
    offset = jump_offset.get<int>();
  }

  size_t get_jump_offset(size_t current_offset) const override {
    (void) current_offset;
    return static_cast<size_t>(offset);
  }

 private:
  static thread_local int offset;
};

thread_local int _jump::offset = 0;

REGISTER_PRIMITIVE(_jump_if_zero);
REGISTER_PRIMITIVE(_jump);

}  // namespace core

}  // namespace bm
