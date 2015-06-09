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

#ifndef _BM_ACTIONS_H_
#define _BM_ACTIONS_H_

#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>

#include <cassert>

#include "phv.h"
#include "named_p4object.h"
#include "packet.h"
#include "event_logger.h"

using std::vector;

// forward declaration of ActionPrimitive_
class ActionPrimitive_;

class ActionOpcodesMap {
public:
  static ActionOpcodesMap *get_instance();
  bool register_primitive(
      const char *name,
      std::unique_ptr<ActionPrimitive_> primitive);

  ActionPrimitive_ *get_primitive(const std::string &name);
private:
  std::unordered_map<std::string, std::unique_ptr<ActionPrimitive_> > map_{};
};

#define REGISTER_PRIMITIVE(primitive_name)\
  bool primitive_name##_create_ =\
    ActionOpcodesMap::get_instance()->register_primitive(\
        #primitive_name,\
        std::unique_ptr<ActionPrimitive_>(new primitive_name()));

struct ActionParam
{
  enum {CONST, FIELD, HEADER, ACTION_DATA, HEADER_STACK} tag;

  union {
    unsigned int const_offset;

    struct {
      header_id_t header;
      int field_offset;
    } field;

    header_id_t header;

    unsigned int action_data_offset;

    header_stack_id_t header_stack;
  };
};

struct ActionData {
  const Data &get(int offset) const { return action_data[offset]; }

  void push_back_action_data(const Data &data) {
    action_data.push_back(data);
  }

  void push_back_action_data(unsigned int data) {
    action_data.emplace_back(data);
  }

  void push_back_action_data(const char *bytes, int nbytes) {
    action_data.emplace_back(bytes, nbytes);
  }

  size_t size() const { return action_data.size(); }

  vector<Data> action_data{};
};

struct ActionEngineState {
  PHV &phv;
  const ActionData &action_data;
  /* const vector<Data> &action_data; */
  const vector<Data> &const_values;

  ActionEngineState(PHV &phv,
		    const ActionData &action_data,
		    const vector<Data> &const_values)
    : phv(phv), action_data(action_data), const_values(const_values) {}
};

struct ActionParamWithState {
  const ActionParam &ap;
  ActionEngineState &state;

  ActionParamWithState(const ActionParam &ap, ActionEngineState &state)
    : ap(ap), state(state) {}

  /* I cannot think of an alternate solution to this. Is there any danger to
     overload cast operators like this ? */

  /* If you want to modify it, don't ask for data..., can I improve this? */
  operator const Data &() {
    switch(ap.tag) {
    case ActionParam::CONST:
      return state.const_values[ap.const_offset];
    case ActionParam::FIELD:
      return state.phv.get_field(ap.field.header, ap.field.field_offset);
    case ActionParam::ACTION_DATA:
      return state.action_data.get(ap.action_data_offset);
    default:
      assert(0);
    }
  }

  operator Field &() {
    assert(ap.tag == ActionParam::FIELD);
    return state.phv.get_field(ap.field.header, ap.field.field_offset);
  }

  operator Header &() {
    assert(ap.tag == ActionParam::HEADER);
    return state.phv.get_header(ap.header);
  }

  operator HeaderStack &() {
    assert(ap.tag == ActionParam::HEADER_STACK);
    return state.phv.get_header_stack(ap.header_stack);
  }
};

/* This is adapted from stack overflow code:
   http://stackoverflow.com/questions/11044504/any-solution-to-unpack-a-vector-to-function-arguments-in-c
*/

template <std::size_t... Indices>
struct indices {
  using next = indices<Indices..., sizeof...(Indices)>;
};

template <std::size_t N>
struct build_indices {
  using type = typename build_indices<N-1>::type::next;
};

template <>
struct build_indices<0> {
  using type = indices<>;
};

template <std::size_t N>
using BuildIndices = typename build_indices<N>::type;

template <size_t num_args>
struct unpack_caller
{
private:
  template <typename T, size_t... I>
  void call(T *pObj, ActionEngineState &state,
	    const ActionParam *args,
	    indices<I...>){
    (*pObj)(ActionParamWithState(args[I], state)...);
  }

public:
  template <typename T>
  void operator () (T* pObj, ActionEngineState &state,
		    const ActionParam *args){
    // assert(args.size() == num_args); // just to be sure
    call(pObj, state, args, BuildIndices<num_args>{});
  }
};

class ActionPrimitive_
{
public:
  virtual ~ActionPrimitive_() { };

  virtual void execute(
      ActionEngineState &state,
      const ActionParam *args) = 0;

  virtual size_t get_num_params() = 0;
};

template <typename... Args>
class ActionPrimitive : public ActionPrimitive_
{
public:
  void execute(ActionEngineState &state, const ActionParam *args) {
    phv = &(state.phv);
    caller(this, state, args);
  }

  // cannot make it constexpr because it is virtual
  // too slow? make it a return value for execute?
  size_t get_num_params() { return sizeof...(Args); }

  virtual void operator ()(Args...) = 0;

protected:
  Field &get_field(std::string name) {
    return phv->get_field(name);
  }

  Header &get_header(std::string name) {
    return phv->get_header(name);
  }

private:
  unpack_caller<sizeof...(Args)> caller;
  PHV *phv;
};

// This is how you declare a primitive:
// class SetField : public ActionPrimitive<Field &, Data &> {
//   void operator ()(Field &f, Data &d) {
//     f.set(d);
//   }
// };

// class Add : public ActionPrimitive<Field &, Data &, Data &> {
//   void operator ()(Field &f, Data &d1, Data &d2) {
//     f.add(d1, d2);
//   }
// };
  
 
// forward declaration
class ActionFnEntry;

class ActionFn : public NamedP4Object
{
  friend class ActionFnEntry;

public:
  ActionFn(const std::string &name, p4object_id_t id)
    : NamedP4Object(name, id) { }

  void parameter_push_back_field(header_id_t header, int field_offset);
  void parameter_push_back_header(header_id_t header);
  void parameter_push_back_header_stack(header_stack_id_t header_stack);
  void parameter_push_back_const(const Data &data);
  void parameter_push_back_action_data(int action_data_offset);

  void push_back_primitive(ActionPrimitive_ *primitive);

private:
  vector<ActionPrimitive_ *> primitives{};
  vector<ActionParam> params{};
  vector<Data> const_values{};
};


class ActionFnEntry
{
public:
  ActionFnEntry() {}

  ActionFnEntry(const ActionFn *action_fn, ActionData action_data)
    : action_fn(action_fn), action_data(std::move(action_data)) { }

  ActionFnEntry(const ActionFn *action_fn)
    : action_fn(action_fn) { }

  void operator()(Packet *pkt) const
  {
    if(!action_fn) return; // happens when no default action specified... TODO
    ELOGGER->action_execute(*pkt, *action_fn, action_data);
    ActionEngineState state(*pkt->get_phv(), action_data,
			    action_fn->const_values);
    auto &primitives = action_fn->primitives;
    size_t param_offset = 0;
    for(auto primitive_it = primitives.begin();
	primitive_it != primitives.end();
	++primitive_it) {
      (*primitive_it)->execute(state, &(action_fn->params[param_offset]));
      param_offset += (*primitive_it)->get_num_params();
    }
  }

  void push_back_action_data(const Data &data) {
    action_data.push_back_action_data(data);
  }

  void push_back_action_data(unsigned int data) {
    action_data.push_back_action_data(data);
  }

  void push_back_action_data(const char *bytes, int nbytes) {
    action_data.push_back_action_data(bytes, nbytes);
  }

  size_t action_data_size() const { return action_data.size(); }

  const Data&get_action_data(int offset) const {
    return action_data.get(offset);
  }

  ActionFnEntry(const ActionFnEntry &other) = default;
  ActionFnEntry &operator=(const ActionFnEntry &other) = default;

  ActionFnEntry(ActionFnEntry &&other) /*noexcept*/ = default;
  ActionFnEntry &operator=(ActionFnEntry &&other) /*noexcept*/ = default;

private:
  const ActionFn *action_fn{nullptr};
  ActionData action_data{};
};

#endif
