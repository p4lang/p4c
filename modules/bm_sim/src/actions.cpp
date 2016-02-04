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

#include <string>

#include "bm_sim/actions.h"
#include "bm_sim/debugger.h"
#include "bm_sim/event_logger.h"

namespace bm {

// the first tmp data register is reserved for internal engine use (register
// index evaluation)
size_t ActionFn::nb_data_tmps = 1;

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
}

void
ActionFn::parameter_push_back_register_gen(
    RegisterArray *register_array, std::unique_ptr<ArithExpression> idx) {
  ActionParam param;
  param.tag = ActionParam::REGISTER_GEN;
  param.register_gen.array = register_array;
  expressions.push_back(std::move(idx));
  param.register_gen.idx = expressions.back().get();
  params.push_back(param);
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
}

void
ActionFn::parameter_push_back_expression(
  std::unique_ptr<ArithExpression> expr
) {
  size_t nb_expression_params = 1;
  for (const ActionParam &p : params)
    if (p.tag == ActionParam::EXPRESSION) nb_expression_params += 1;

  assert(nb_expression_params <= ActionFn::nb_data_tmps);
  if (nb_expression_params == ActionFn::nb_data_tmps)
    ActionFn::nb_data_tmps += 1;

  expressions.push_back(std::move(expr));
  ActionParam param;
  param.tag = ActionParam::EXPRESSION;
  param.expression = {static_cast<unsigned int>(nb_expression_params),
                      expressions.back().get()};
  params.push_back(param);
}

void
ActionFn::push_back_primitive(ActionPrimitive_ *primitive) {
  primitives.push_back(primitive);
}


bool
ActionOpcodesMap::register_primitive(
    const char *name,
    std::unique_ptr<ActionPrimitive_> primitive) {
  const std::string str_name = std::string(name);
  auto it = map_.find(str_name);
  if (it != map_.end()) return false;
  map_[str_name] = std::move(primitive);
  return true;
}

ActionPrimitive_ *
ActionOpcodesMap::get_primitive(const std::string &name) {
  return map_[name].get();
}

ActionOpcodesMap *
ActionOpcodesMap::get_instance() {
  static ActionOpcodesMap instance;
  return &instance;
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
ActionFnEntry::operator()(Packet *pkt) const {
  // happens when no default action specified... TODO(antonin)
  if (!action_fn) return;
  BMELOG(action_execute, *pkt, *action_fn, action_data);
  // TODO(antonin)
  // this is temporary while we experiment with the debugger
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_ACTION | action_fn->get_id());
  ActionEngineState state(pkt, action_data, action_fn->const_values);
  auto &primitives = action_fn->primitives;
  size_t param_offset = 0;
  // primitives is a vector of pointers
  for (auto primitive : primitives) {
    primitive->execute(&state, &(action_fn->params[param_offset]));
    param_offset += primitive->get_num_params();
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
  std::ios_base::fmtflags ff;
  ff = std::cout.flags();
  for (const Data &d : action_data.action_data)
    (*stream) << std::hex << d << ",";
  stream->flags(ff);
}

}  // namespace bm
