#ifndef _BM_ACTIONS_H_
#define _BM_ACTIONS_H_

#include <vector>
#include <functional>

#include <cassert>

#include "phv.h"

using std::vector;

// forward declaration of ActionStack
class ActionStack;

typedef std::function<void(PHV &, ActionStack &)> ActionPrimitive;

struct ActionParam
{
  enum {CONST, FIELD, HEADER, ACTION_DATA} tag;

  union {
    unsigned int const_offset;

    struct {
      header_id_t header;
      int field_offset;
    } field;

    header_id_t header;

    unsigned int action_data_offset;
  };
};

class ActionStack
{
public:
  virtual const Data &pop_const_data(PHV &phv) = 0;
  virtual Field &pop_field(PHV &phv) = 0;
  virtual Header &pop_header(PHV &phv) = 0;
  // TODO: stateful

protected:
  int param_idx{0};
};
 

class ActionFn
{
public:
  void parameter_push_back_field(header_id_t header, int field_offset);
  void parameter_push_back_header(header_id_t header);
  void parameter_push_back_const(const Data &data);
  void parameter_push_back_action_data(int action_data_offset);

  const vector<ActionPrimitive> &get_primitives() const { return primitives; }
  const vector<ActionParam> &get_params() const { return params; }

  const Data &get_const_value(int const_offset) const {
    return const_values[const_offset];
  }

private:
  vector<ActionPrimitive> primitives;
  vector<ActionParam> params;
  vector<Data> const_values;
};

class ActionFnEntry : public ActionStack
{
public:
  ActionFnEntry() {}

  ActionFnEntry(const ActionFn *action_fn, unsigned int action_data_cnt = 0)
    : action_fn(action_fn) {
    action_data.reserve(action_data_cnt);
  }

  void operator()(PHV &phv)
  {
    param_idx = 0;
    auto &primitives = action_fn->get_primitives();
    for(auto primitive_it = primitives.begin();
	primitive_it != primitives.end();
	++primitive_it) {
      (*primitive_it)(phv, *this);
    }
  }

  void push_back_action_data(const Data &data) {
    action_data.push_back(data);
  }

  void push_back_action_data(unsigned int data) {
    action_data.emplace_back(data);
  }

  void push_back_action_data(const char *bytes, int nbytes) {
    action_data.emplace_back(bytes, nbytes);
  }

  const Data &pop_const_data(PHV &phv) {
    const auto &params = action_fn->get_params();
    const ActionParam &param = params[param_idx++];
    switch(param.tag) {
    case ActionParam::CONST:
      return action_fn->get_const_value(param.const_offset);
    case ActionParam::FIELD:
      return phv.get_field(param.field.header, param.field.field_offset);
    case ActionParam::ACTION_DATA:
      return action_data[param.action_data_offset];
    default:
      assert(0);
    }
  }

  Field &pop_field(PHV &phv) {
    const auto &params = action_fn->get_params();
    const ActionParam &param = params[param_idx++];
    assert(param.tag == ActionParam::FIELD);
    return phv.get_field(param.field.header, param.field.field_offset);
  }

  Header &pop_header(PHV &phv) {
    const auto &params = action_fn->get_params();
    const ActionParam &param = params[param_idx++];
    assert(param.tag == ActionParam::HEADER);
    return phv.get_header(param.header);
  }

private:
  const ActionFn *action_fn{nullptr};
  vector<Data> action_data;
};

#endif
