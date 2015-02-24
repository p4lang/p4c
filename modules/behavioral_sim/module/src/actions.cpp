#include "actions.h"

void ActionFn::parameter_push_back_field(header_id_t header, int field_offset) {
  ActionParam param;
  param.tag = ActionParam::FIELD;
  param.field = {header, field_offset};
  params.push_back(param);
}

void ActionFn::parameter_push_back_header(header_id_t header) {
  ActionParam param;
  param.tag = ActionParam::HEADER;
  param.header = header;;
  params.push_back(param);
}

void ActionFn::parameter_push_back_const(const Data &data) {
  const_values.push_back(data);
  ActionParam param;
  param.tag = ActionParam::CONST;
  param.const_offset = const_values.size() - 1;;
  params.push_back(param);
}

void ActionFn::parameter_push_back_action_data(int action_data_offset) {
  ActionParam param;
  param.tag = ActionParam::ACTION_DATA;
  param.action_data_offset = action_data_offset;
  params.push_back(param);
}

void ActionFn::push_back_primitive(unique_ptr<ActionPrimitive_> primitive) {
  primitives.push_back(std::move(primitive));
}
