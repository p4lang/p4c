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

//! @file actions.h
//!
//! Each action primitive supported by the target needs to be declared as a
//! functor. This is how you declare one:
//! @code
//! class SetField :  public ActionPrimitive<Field &, const Data &> {
//!   void operator ()(Field &f, const Data &d) {
//!     f.set(d);
//!   }
//! };
//!
//! class Add :  public ActionPrimitive<Field &, const Data &, const Data &> {
//!   void operator ()(Field &f, const Data &d1, const Data &d2) {
//!     f.add(d1, d2);
//!   }
//! };
//! @endcode
//!
//! You can then register them like this:
//! @code
//! REGISTER_PRIMITIVE(SetField)
//! REGISTER_PRIMITIVE(Add)
//! @endcode
//! And use them by their name in the P4 program / JSON file:
//! @code
//! e.g. SetField(hdrA.f1, 99);
//! @endcode
//!
//! Valid functor parameters for an action primitive include:
//!   - `const Data &` for any P4 numerical values (it can be a P4 constant, a
//! field, action data or the result of an expression).
//!   - `Data &` for a writable P4 numerical value (essentially a field)
//!   - `[const] Field &` for P4 field references
//!   - `[const] Header &` for P4 header references
//!   - `const NamedCalculation &` for P4 field list calculations
//!   - `[const] MeterArray &`
//!   - `[const] CounterArray &`
//!   - `[const] RegisterArray &`
//!
//! You can declare and register primitives anywhere in your switch target C++
//! code.


#ifndef BM_BM_SIM_ACTIONS_H_
#define BM_BM_SIM_ACTIONS_H_

#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>
#include <limits>
#include <tuple>

#include <cassert>

#include "phv.h"
#include "named_p4object.h"
#include "packet.h"
#include "calculations.h"
#include "meters.h"
#include "counters.h"
#include "stateful.h"
#include "expressions.h"

namespace bm {

class P4Objects;  // forward declaration for deserialize

// forward declaration of ActionPrimitive_
class ActionPrimitive_;

// Maps primitive names to primitive implementations (functors).
// Targets are responsible for defining their own primitives and registering
// them in this map using the REGISTER_PRIMITIVE(primitive_name) macro.
class ActionOpcodesMap {
 public:
  static ActionOpcodesMap *get_instance();
  bool register_primitive(
      const char *name,
      std::unique_ptr<ActionPrimitive_> primitive);

  ActionPrimitive_ *get_primitive(const std::string &name);
 private:
  // Maps primitive names to their implementation.
  // The same pointer is used system-wide, even if a primitive is called from
  // different actions. As such, one has to be careful if some state is
  // maintained byt the primitive functor.
  std::unordered_map<std::string, std::unique_ptr<ActionPrimitive_> > map_{};
};

//! When implementing an action primitive for a target, this macro needs to be
//! called to make this module aware of the primitive existence.
#define REGISTER_PRIMITIVE(primitive_name)                              \
  bool primitive_name##_create_ =                                       \
      bm::ActionOpcodesMap::get_instance()->register_primitive(         \
          #primitive_name,                                              \
          std::unique_ptr<bm::ActionPrimitive_>(new primitive_name()));

//! This macro can also be called when registering a primitive for a target, in
//! case you want to register the primitive under a different name than its
//! functor name
#define REGISTER_PRIMITIVE_W_NAME(primitive_name, primitive)            \
  bool primitive##_create_ =                                            \
      bm::ActionOpcodesMap::get_instance()->register_primitive(         \
          primitive_name,                                               \
          std::unique_ptr<bm::ActionPrimitive_>(new primitive()));

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

  std::vector<Data> action_data{};
};

struct ActionEngineState {
  Packet &pkt;
  PHV &phv;
  const ActionData &action_data;
  const std::vector<Data> &const_values;

  ActionEngineState(Packet *pkt,
                    const ActionData &action_data,
                    const std::vector<Data> &const_values)
    : pkt(*pkt), phv(*pkt->get_phv()),
      action_data(action_data), const_values(const_values) {}
};

class ExternType;

// A simple tagged union, which holds the definition of an action parameter. For
// example, if an action parameter is a field, this struct will hold the header
// id and the offset for this field. If an action parameter is a meter array,
// the struct will hold a raw, non-owning pointer to the MeterArray object.
struct ActionParam {
  // some old P4 primitives take a calculation as a parameter, I don't know if I
  // will keep it around but for now I need it
  enum {CONST, FIELD, HEADER, ACTION_DATA, REGISTER_REF, REGISTER_GEN,
        HEADER_STACK, CALCULATION,
        METER_ARRAY, COUNTER_ARRAY, REGISTER_ARRAY,
        EXPRESSION,
        EXTERN_INSTANCE} tag;

  union {
    unsigned int const_offset;

    struct {
      header_id_t header;
      int field_offset;
    } field;

    header_id_t header;

    unsigned int action_data_offset;

    // In theory, if registers cannot be resized, I could directly store a
    // pointer to the correct register cell, i.e. &(*array)[idx]. However, this
    // gives me more flexibility in case I want to be able to resize the
    // registers arbitrarily in the future.
    struct {
      RegisterArray *array;
      unsigned int idx;
    } register_ref;

    struct {
      RegisterArray *array;
      ArithExpression *idx;
    } register_gen;

    header_stack_id_t header_stack;

    // non owning pointer
    const NamedCalculation *calculation;

    // non owning pointer
    MeterArray *meter_array;

    // non owning pointer
    CounterArray *counter_array;

    // non owning pointer
    RegisterArray *register_array;

    struct {
      unsigned int offset;
      // non owning pointer
      // in theory, could be an owning pointer, but the union makes this
      // complicated, so instead the ActionFn keeps a vector of owning pointers
      ArithExpression *ptr;
    } expression;

    ExternType *extern_instance;
  };

  // convert to the correct type when calling a primitive
  template <typename T> T to(ActionEngineState *state) const;
};

// template specializations for ActionParam "casting"
// they have to be declared outside of the class declaration, and "inline" is
// necessary to avoid linker errors

// can only be a field or a register reference
template <> inline
Data &ActionParam::to<Data &>(ActionEngineState *state) const {
  static thread_local Data data_temp;

  switch (tag) {
    case ActionParam::FIELD:
      return state->phv.get_field(field.header, field.field_offset);
    case ActionParam::REGISTER_REF:
      return register_ref.array->at(register_ref.idx);
    case ActionParam::REGISTER_GEN:
      register_gen.idx->eval(state->phv, &data_temp,
                             state->action_data.action_data);
      return register_ref.array->at(data_temp.get<size_t>());
    default:
      assert(0);
  }
}

template <> inline
const Data &ActionParam::to<const Data &>(ActionEngineState *state) const {
  // thread_local sounds like a good choice here
  // alternative would be to have one vector for each ActionFnEntry
  static thread_local unsigned int data_temps_size = 4;
  static thread_local std::vector<Data> data_temps(data_temps_size);

  switch (tag) {
    case ActionParam::CONST:
      return state->const_values[const_offset];
    case ActionParam::FIELD:
      return state->phv.get_field(field.header, field.field_offset);
    case ActionParam::ACTION_DATA:
      return state->action_data.get(action_data_offset);
    case ActionParam::REGISTER_REF:
      return register_ref.array->at(register_ref.idx);
    case ActionParam::REGISTER_GEN:
      register_gen.idx->eval(state->phv, &data_temps[0],
                             state->action_data.action_data);
      return register_ref.array->at(data_temps[0].get<size_t>());
    case ActionParam::EXPRESSION:
      while (data_temps_size <= expression.offset) {
        data_temps.emplace_back();
        data_temps_size++;
      }
      expression.ptr->eval(state->phv, &data_temps[expression.offset],
                              state->action_data.action_data);
      return data_temps[expression.offset];
    default:
      assert(0);
  }
}

// TODO(antonin): maybe I should use meta-programming for const type handling,
// but I cannot think of an easy way, so I am leaving it as is for now

template <> inline
Field &ActionParam::to<Field &>(ActionEngineState *state) const {
  assert(tag == ActionParam::FIELD);
  return state->phv.get_field(field.header, field.field_offset);
}

template <> inline
const Field &ActionParam::to<const Field &>(ActionEngineState *state) const {
  return ActionParam::to<Field &>(state);
}

template <> inline
Header &ActionParam::to<Header &>(ActionEngineState *state) const {
  assert(tag == ActionParam::HEADER);
  return state->phv.get_header(header);
}

template <> inline
const Header &ActionParam::to<const Header &>(ActionEngineState *state) const {
  return ActionParam::to<Header &>(state);
}

template <> inline
HeaderStack &ActionParam::to<HeaderStack &>(ActionEngineState *state) const {
  assert(tag == ActionParam::HEADER_STACK);
  return state->phv.get_header_stack(header_stack);
}

template <> inline
const HeaderStack &ActionParam::to<const HeaderStack &>(
    ActionEngineState *state) const {
  return ActionParam::to<HeaderStack &>(state);
}

template <> inline
const NamedCalculation &ActionParam::to<const NamedCalculation &>(
    ActionEngineState *state) const {
  (void) state;
  assert(tag == ActionParam::CALCULATION);
  return *(calculation);
}

template <> inline
MeterArray &ActionParam::to<MeterArray &>(ActionEngineState *state) const {
  (void) state;
  assert(tag == ActionParam::METER_ARRAY);
  return *(meter_array);
}

template <> inline
const MeterArray &ActionParam::to<const MeterArray &>(
    ActionEngineState *state) const {
  return ActionParam::to<MeterArray &>(state);
}

template <> inline
CounterArray &ActionParam::to<CounterArray &>(ActionEngineState *state) const {
  (void) state;
  assert(tag == ActionParam::COUNTER_ARRAY);
  return *(counter_array);
}

template <> inline
const CounterArray &ActionParam::to<const CounterArray &>(
    ActionEngineState *state) const {
  return ActionParam::to<CounterArray &>(state);
}

template <> inline
RegisterArray &ActionParam::to<RegisterArray &>(
    ActionEngineState *state) const {
  (void) state;
  assert(tag == ActionParam::REGISTER_ARRAY);
  return *(register_array);
}

template <> inline
const RegisterArray &ActionParam::to<const RegisterArray &>(
    ActionEngineState *state) const {
  return ActionParam::to<RegisterArray &>(state);
}

template <> inline
ExternType *ActionParam::to<ExternType *>(ActionEngineState *state) const {
  (void) state;
  assert(tag == ActionParam::EXTERN_INSTANCE);
  return extern_instance;
}

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

template <typename... Args>
struct unpack_caller {
 private:
  template <typename T, size_t... I>
  void call(T *pObj, ActionEngineState *state,
            const ActionParam *args,
            const indices<I...>) {
    // I guess we can have an unused parameter warning if an instantiation has
    // an empty indices list
    (void) state; (void) args;
#define ELEM_TYPE typename std::tuple_element<I, std::tuple<Args...>>::type
    (*pObj)(args[I].to<ELEM_TYPE>(state)...);
#undef ELEM_TYPE
  }

 public:
  template <typename T>
  void operator () (T* pObj, ActionEngineState *state,
                    const ActionParam *args){
    // assert(args.size() == sizeof...(Args)); // just to be sure
    call(pObj, state, args, BuildIndices<sizeof...(Args)>{});
  }
};

class ActionPrimitive_ {
 public:
  virtual ~ActionPrimitive_() { }

  virtual void execute(
      ActionEngineState *state,
      const ActionParam *args) = 0;

  virtual size_t get_num_params() = 0;

 protected:
  // This used to be regular members in ActionPrimitive, but there could be a
  // race condition. Making them thread_local solves the issue. I moved these
  // from ActionPrimitive because g++4.8 has a bug when using thread_local class
  // variables with templates
  // (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60056).
  static thread_local Packet *pkt;
  static thread_local PHV *phv;
};

//! This acts as the base class for all target-specific action primitives. It
//! provides some useful functionality, like access to a header by name, or
//! access to the Packet on which the primitive is executed.
template <typename... Args>
class ActionPrimitive :  public ActionPrimitive_ {
 public:
  void execute(ActionEngineState *state, const ActionParam *args) {
    phv = &(state->phv);
    pkt = &(state->pkt);
    caller(this, state, args);
  }

  // cannot make it constexpr because it is virtual
  size_t get_num_params() { return sizeof...(Args); }

  virtual void operator ()(Args...) = 0;

 protected:
  //! Returns a reference to the Field with name \p name. \p name must follow
  //! the usual format `"hdrA.f1"`.
  Field &get_field(std::string name) {
    return phv->get_field(name);
  }

  //! Returns a reference to the Header with name \p name.
  Header &get_header(std::string name) {
    return phv->get_header(name);
  }

  // in practice, regular primitives should not have to use this
  PHV &get_phv() {
    return *phv;
  }

  // so far used only for meter arrays and counter arrays
  //! Returns a reference to the Packet instance on which the action primitive
  //! is being executed.
  Packet &get_packet() {
    return *pkt;
  }

 private:
  unpack_caller<Args...> caller;
  // See thread_local members in ActionPrimitive_ class
  // PHV *phv;
  // Packet *pkt;
};


// forward declaration
class ActionFnEntry;

class ActionFn :  public NamedP4Object {
  friend class ActionFnEntry;

 public:
  ActionFn(const std::string &name, p4object_id_t id)
    : NamedP4Object(name, id) { }

  void parameter_push_back_field(header_id_t header, int field_offset);
  void parameter_push_back_header(header_id_t header);
  void parameter_push_back_header_stack(header_stack_id_t header_stack);
  void parameter_push_back_const(const Data &data);
  void parameter_push_back_action_data(int action_data_offset);
  void parameter_push_back_register_ref(RegisterArray *register_array,
                                        unsigned int idx);
  void parameter_push_back_register_gen(RegisterArray *register_array,
                                        std::unique_ptr<ArithExpression> idx);
  void parameter_push_back_calculation(const NamedCalculation *calculation);
  void parameter_push_back_meter_array(MeterArray *meter_array);
  void parameter_push_back_counter_array(CounterArray *counter_array);
  void parameter_push_back_register_array(RegisterArray *register_array);
  void parameter_push_back_expression(std::unique_ptr<ArithExpression> expr);
  void parameter_push_back_extern_instance(ExternType *extern_instance);

  void push_back_primitive(ActionPrimitive_ *primitive);

  // TODO(antonin)
  // size_t num_params() const { 0u; }

 private:
  std::vector<ActionPrimitive_ *> primitives{};
  std::vector<ActionParam> params{};
  RegisterSync register_sync{};
  std::vector<Data> const_values{};
  // should I store the objects in the vector, instead of pointers?
  std::vector<std::unique_ptr<ArithExpression> > expressions{};

 private:
  static size_t nb_data_tmps;
};


class ActionFnEntry {
 public:
  ActionFnEntry() { }

  ActionFnEntry(const ActionFn *action_fn, ActionData action_data)
    : action_fn(action_fn), action_data(std::move(action_data)) { }

  explicit ActionFnEntry(const ActionFn *action_fn)
    : action_fn(action_fn) { }

  void operator()(Packet *pkt) const;

  void push_back_action_data(const Data &data);

  void push_back_action_data(unsigned int data);

  void push_back_action_data(const char *bytes, int nbytes);

  size_t action_data_size() const { return action_data.size(); }

  const Data &get_action_data_at(int offset) const {
    return action_data.get(offset);
  }

  const ActionData &get_action_data() const {
    return action_data;
  }

  void dump(std::ostream *stream) const;

  void serialize(std::ostream *out) const;
  void deserialize(std::istream *in, const P4Objects &objs);

  p4object_id_t get_action_id() const {
    if (!action_fn) return std::numeric_limits<p4object_id_t>::max();
    return action_fn->get_id();
  }

  const ActionFn *get_action_fn() const {
    return action_fn;
  }

  ActionFnEntry(const ActionFnEntry &other) = default;
  ActionFnEntry &operator=(const ActionFnEntry &other) = default;

  ActionFnEntry(ActionFnEntry &&other) /*noexcept*/ = default;
  ActionFnEntry &operator=(ActionFnEntry &&other) /*noexcept*/ = default;

 private:
  const ActionFn *action_fn{nullptr};
  ActionData action_data{};
};

}  // namespace bm

#endif  // BM_BM_SIM_ACTIONS_H_
