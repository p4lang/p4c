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
//!   - `[const] HeaderUnion &`
//!   - `[const] StackIface &` for arbitrary P4 stacks
//!   - `[const] HeaderStack &` for P4 header stacks
//!   - `[const] HeaderUnionStack &` for P4 header union stacks
//!   - `const std::string &` or `const char *` for strings
//!   - `const std::vector<Data>` for lists of numerical values
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
#include <iosfwd>
#include <limits>
#include <tuple>

#include <cassert>

#include "phv.h"
#include "named_p4object.h"
#include "expressions.h"
#include "stateful.h"
#include "_assert.h"

namespace bm {

class P4Objects;  // forward declaration for deserialize

// some forward declarations for needed p4 objects
class Packet;
class NamedCalculation;
class MeterArray;
class CounterArray;

// forward declaration of ActionPrimitive_
class ActionPrimitive_;

// Maps primitive names to primitive implementations (functors).
// Targets are responsible for defining their own primitives and registering
// them in this map using the REGISTER_PRIMITIVE(primitive_name) macro.
class ActionOpcodesMap {
 public:
  using ActionPrimitiveFactoryFn =
      std::function<std::unique_ptr<ActionPrimitive_>()>;
  static ActionOpcodesMap *get_instance();
  bool register_primitive(const char *name, ActionPrimitiveFactoryFn primitive);

  std::unique_ptr<ActionPrimitive_> get_primitive(const std::string &name);
 private:
  ActionOpcodesMap();

  // Maps primitive names to their implementation.
  // The same pointer is used system-wide, even if a primitive is called from
  // different actions. As such, one has to be careful if some state is
  // maintained byt the primitive functor.
  std::unordered_map<std::string, ActionPrimitiveFactoryFn> map_{};
};

//! When implementing an action primitive for a target, this macro needs to be
//! called to make this module aware of the primitive existence.
#define REGISTER_PRIMITIVE(primitive_name)                                  \
  bool primitive_name##_create_ =                                           \
      bm::ActionOpcodesMap::get_instance()->register_primitive(             \
          #primitive_name,                                                  \
          [](){ return std::unique_ptr<::bm::ActionPrimitive_>(             \
              new primitive_name()); })

//! This macro can also be called when registering a primitive for a target, in
//! case you want to register the primitive under a different name than its
//! functor name
#define REGISTER_PRIMITIVE_W_NAME(primitive_name, primitive)            \
  bool primitive##_create_ =                                            \
      bm::ActionOpcodesMap::get_instance()->register_primitive(         \
          primitive_name,                                               \
          [](){ return std::unique_ptr<::bm::ActionPrimitive_>(         \
              new primitive()); })

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

struct ActionEngineState;

class ExternType;

// A simple tagged union, which holds the definition of an action parameter. For
// example, if an action parameter is a field, this struct will hold the header
// id and the offset for this field. If an action parameter is a meter array,
// the struct will hold a raw, non-owning pointer to the MeterArray object.
struct ActionParam {
  // some old P4 primitives take a calculation as a parameter, I don't know if I
  // will keep it around but for now I need it
  enum {CONST, FIELD, HEADER, ACTION_DATA, REGISTER_REF, REGISTER_GEN,
        HEADER_STACK, LAST_HEADER_STACK_FIELD,
        CALCULATION,
        METER_ARRAY, COUNTER_ARRAY, REGISTER_ARRAY,
        EXPRESSION, EXPRESSION_HEADER, EXPRESSION_HEADER_STACK,
        EXPRESSION_HEADER_UNION, EXPRESSION_HEADER_UNION_STACK,
        EXTERN_INSTANCE,
        STRING,
        HEADER_UNION, HEADER_UNION_STACK, PARAMS_VECTOR} tag;

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

    // for lists of numerical values, used for the log_msg primitive and nothing
    // else at the moment
    struct {
      unsigned int start;
      unsigned int end;
    } params_vector;

    // special case when trying to access a field in the last header of a stack
    struct {
      header_stack_id_t header_stack;
      int field_offset;
    } stack_field;

    // non owning pointer
    const NamedCalculation *calculation;

    // non owning pointer
    MeterArray *meter_array;

    // non owning pointer
    CounterArray *counter_array;

    // non owning pointer
    RegisterArray *register_array;

    struct {
      unsigned int offset;  // only used for arithmetic ("Data") expressions
      // non owning pointer
      // in theory, could be an owning pointer, but the union makes this
      // complicated, so instead the ActionFn keeps a vector of owning pointers
      Expression *ptr;
    } expression;

    ExternType *extern_instance;

    // I use a pointer here to avoid complications with the union; the string
    // memory is owned by ActionFn (just like for Expression above)
    const std::string *str;

    header_union_stack_id_t header_union_stack;

    header_union_id_t header_union;
  };

  // convert to the correct type when calling a primitive
  template <typename T> T to(ActionEngineState *state) const;
};

struct ActionEngineState {
  Packet &pkt;
  PHV &phv;
  const ActionData &action_data;
  const std::vector<Data> &const_values;
  const std::vector<ActionParam> &parameters_vector;

  ActionEngineState(Packet *pkt,
                    const ActionData &action_data,
                    const std::vector<Data> &const_values,
                    const std::vector<ActionParam> &parameters_vector);
};

// template specializations for ActionParam "casting"
// they have to be declared outside of the class declaration, and "inline" is
// necessary to avoid linker errors

template <> inline
Data &ActionParam::to<Data &>(ActionEngineState *state) const {
  static thread_local Data data_temp;

  switch (tag) {
    case ActionParam::FIELD:
      return state->phv.get_field(field.header, field.field_offset);
    case ActionParam::REGISTER_REF:
      return register_ref.array->at(register_ref.idx);
    case ActionParam::REGISTER_GEN:
      register_gen.idx->eval_arith(state->phv, &data_temp,
                                   state->action_data.action_data);
      return register_ref.array->at(data_temp.get<size_t>());
    case ActionParam::EXPRESSION:
      return expression.ptr->eval_arith_lvalue(&state->phv,
                                               state->action_data.action_data);
    case ActionParam::LAST_HEADER_STACK_FIELD:
      return state->phv.get_header_stack(stack_field.header_stack).get_last()
          .get_field(stack_field.field_offset);
    default:
      _BM_UNREACHABLE("Default switch case should not be reachable");
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
      register_gen.idx->eval_arith(state->phv, &data_temps[0],
                                   state->action_data.action_data);
      return register_ref.array->at(data_temps[0].get<size_t>());
    case ActionParam::EXPRESSION:
      while (data_temps_size <= expression.offset) {
        data_temps.emplace_back();
        data_temps_size++;
      }
      expression.ptr->eval_arith(state->phv, &data_temps[expression.offset],
                                 state->action_data.action_data);
      return data_temps[expression.offset];
    case ActionParam::LAST_HEADER_STACK_FIELD:
      return state->phv.get_header_stack(stack_field.header_stack).get_last()
          .get_field(stack_field.field_offset);
    default:
      _BM_UNREACHABLE("Default switch case should not be reachable");
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
  switch (tag) {
    case ActionParam::HEADER:
      return state->phv.get_header(header);
    case ActionParam::EXPRESSION_HEADER:
      return expression.ptr->eval_header(&state->phv,
                                         state->action_data.action_data);
    default:
      _BM_UNREACHABLE("Default switch case should not be reachable");
  }
}

template <> inline
const Header &ActionParam::to<const Header &>(ActionEngineState *state) const {
  return ActionParam::to<Header &>(state);
}

template <> inline
HeaderStack &ActionParam::to<HeaderStack &>(ActionEngineState *state) const {
  switch (tag) {
    case ActionParam::HEADER_STACK:
      return state->phv.get_header_stack(header_stack);
    case ActionParam::EXPRESSION_HEADER_STACK:
      return expression.ptr->eval_header_stack(&state->phv,
                                               state->action_data.action_data);
    default:
      _BM_UNREACHABLE("Default switch case should not be reachable");
  }
}

template <> inline
const HeaderStack &ActionParam::to<const HeaderStack &>(
    ActionEngineState *state) const {
  return ActionParam::to<HeaderStack &>(state);
}

template <> inline
StackIface &ActionParam::to<StackIface &>(ActionEngineState *state) const {
  switch (tag) {
    case HEADER_STACK:
      return state->phv.get_header_stack(header_stack);
    case ActionParam::EXPRESSION_HEADER_STACK:
      return expression.ptr->eval_header_stack(
          &state->phv, state->action_data.action_data);
    case HEADER_UNION_STACK:
      return state->phv.get_header_union_stack(header_union_stack);
    case ActionParam::EXPRESSION_HEADER_UNION_STACK:
      return expression.ptr->eval_header_union_stack(
          &state->phv, state->action_data.action_data);
    default:
      _BM_UNREACHABLE("Default switch case should not be reachable");
  }
}

template <> inline
const StackIface &ActionParam::to<const StackIface &>(
    ActionEngineState *state) const {
  return ActionParam::to<StackIface &>(state);
}

template <> inline
HeaderUnion &ActionParam::to<HeaderUnion &>(ActionEngineState *state) const {
  switch (tag) {
    case ActionParam::HEADER_UNION:
      return state->phv.get_header_union(header_union);
    case ActionParam::EXPRESSION_HEADER_UNION:
      return expression.ptr->eval_header_union(&state->phv,
                                               state->action_data.action_data);
    default:
      _BM_UNREACHABLE("Default switch case should not be reachable");
  }
}

template <> inline
const HeaderUnion &ActionParam::to<const HeaderUnion &>(
    ActionEngineState *state) const {
  return ActionParam::to<HeaderUnion &>(state);
}

template <> inline
HeaderUnionStack &ActionParam::to<HeaderUnionStack &>(
    ActionEngineState *state) const {
  switch (tag) {
    case ActionParam::HEADER_UNION_STACK:
      return state->phv.get_header_union_stack(header_union_stack);
    case ActionParam::EXPRESSION_HEADER_UNION_STACK:
      return expression.ptr->eval_header_union_stack(
          &state->phv, state->action_data.action_data);
    default:
      _BM_UNREACHABLE("Default switch case should not be reachable");
  }
}

template <> inline
const HeaderUnionStack &ActionParam::to<const HeaderUnionStack &>(
    ActionEngineState *state) const {
  return ActionParam::to<HeaderUnionStack &>(state);
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

template <> inline
const std::string &ActionParam::to<const std::string &>(
    ActionEngineState *state) const {
  assert(tag == ActionParam::STRING);
  (void) state;
  return *str;
}

// just a convenience function, I expect the version above to be the most used
// one
template <> inline
const char *ActionParam::to<const char *>(ActionEngineState *state) const {
  assert(tag == ActionParam::STRING);
  (void) state;
  return str->c_str();
}

// used for primitives that take as a parameter a "list" of values. Currently we
// only support "const std::vector<Data>" as the type used by the C++ primitive
// and currently this is only used by log_msg.
// TODO(antonin): more types (e.g. "const std::vector<const Data &>") if needed
// we can support list of other value types as well, e.g. list of headers
// we are not limited to using a vector as the data structure either
template <> inline
const std::vector<Data>
ActionParam::to<const std::vector<Data>>(ActionEngineState *state) const {
  _BM_ASSERT(tag == ActionParam::PARAMS_VECTOR && "not a params vector");
  std::vector<Data> vec;

  for (auto i = params_vector.start ; i < params_vector.end ; i++) {
    // re-use previously-defined cast method; note that we use to<const Data &>
    // and not to<const Data>, as it does not exists
    // if something in the parameters_vector cannot be cast to "const Data &",
    // the code will assert
    vec.push_back(state->parameters_vector[i].to<const Data &>(state));
  }

  return vec;
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

  virtual size_t get_num_params() const = 0;

  virtual size_t get_jump_offset(size_t current_offset) const {
    return current_offset + 1;
  }

  void _set_p4objects(P4Objects *p4objects) {
    this->p4objects = p4objects;
  }

  void set_source_info(SourceInfo *source_info) {
    call_source_info = source_info;
  }

 protected:
  // This used to be regular members in ActionPrimitive, but there could be a
  // race condition. Making them thread_local solves the issue. I moved these
  // from ActionPrimitive because g++4.8 has a bug when using thread_local class
  // variables with templates
  // (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60056).
  static thread_local Packet *pkt;
  static thread_local PHV *phv;
  SourceInfo *call_source_info{nullptr};

  P4Objects *get_p4objects() {
    return p4objects;
  }

 private:
  P4Objects *p4objects{nullptr};
};

//! This acts as the base class for all target-specific action primitives. It
//! provides some useful functionality, like access to a header by name, or
//! access to the Packet on which the primitive is executed.
template <typename... Args>
class ActionPrimitive :  public ActionPrimitive_ {
 public:
  void execute(ActionEngineState *state, const ActionParam *args) override {
    phv = &(state->phv);
    pkt = &(state->pkt);
    caller(this, state, args);
  }

  // cannot make it constexpr because it is virtual
  size_t get_num_params() const override { return sizeof...(Args); }

  virtual void operator ()(Args...) = 0;

 protected:
  //! Returns a reference to the Field with name \p name. \p name must follow
  //! the usual format `"hdrA.f1"`.
  Field &get_field(const std::string &name) {
    return phv->get_field(name);
  }

  //! Returns a reference to the Header with name \p name.
  Header &get_header(const std::string &name) {
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

class ActionPrimitiveCall {
 public:
  explicit ActionPrimitiveCall(
      ActionPrimitive_ *primitive, size_t param_offset,
      std::unique_ptr<SourceInfo> source_info = nullptr);

  void execute(ActionEngineState *state, const ActionParam *args) const;

  size_t get_num_params() const;

  size_t get_param_offset() const { return param_offset; }

  size_t get_jump_offset(size_t current_offset) const {
    return primitive->get_jump_offset(current_offset);
  }

  SourceInfo *get_source_info() const { return source_info.get(); }

 private:
  ActionPrimitive_ *primitive;
  size_t param_offset;
  std::unique_ptr<SourceInfo> source_info;
};

class ActionFn :  public NamedP4Object {
  friend class ActionFnEntry;

 public:
  ActionFn(const std::string &name, p4object_id_t id, size_t num_params);

  ActionFn(const std::string &name, p4object_id_t id, size_t num_params,
           std::unique_ptr<SourceInfo> source_info);

  // these parameter_push_back_* methods are not very well named. They are used
  // to push arguments to the primitives; and are independent of the actual
  // parameters for the P4 action
  void parameter_push_back_field(header_id_t header, int field_offset);
  void parameter_push_back_header(header_id_t header);
  void parameter_push_back_header_stack(header_stack_id_t header_stack);
  void parameter_push_back_last_header_stack_field(
      header_stack_id_t header_stack, int field_offset);
  void parameter_push_back_header_union(header_union_id_t header_union);
  void parameter_push_back_header_union_stack(
      header_union_stack_id_t header_union_stack);
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
  void parameter_push_back_expression(std::unique_ptr<Expression> expr);
  void parameter_push_back_expression(std::unique_ptr<Expression> expr,
                                      ExprType expr_type);
  void parameter_push_back_extern_instance(ExternType *extern_instance);
  void parameter_push_back_string(const std::string &str);

  // These methods are used when we need to push a "vector of parameters"
  // (i.e. when a primitive, such as log_msg, uses a list of values as one of
  // its parameters). First parameter_start_vector is called to signal the
  // beginning of the "vector of parameters", then the parameter_push_back_*
  // methods are called as usual, and finally parameter_end_vector is called to
  // signal the end.
  void parameter_start_vector();
  void parameter_end_vector();

  void push_back_primitive(ActionPrimitive_ *primitive,
                           std::unique_ptr<SourceInfo> source_info = nullptr);

  void grab_register_accesses(RegisterSync *register_sync) const;

  size_t get_num_params() const;

 private:
  using ParameterList = std::vector<ActionParam>;

  std::vector<ActionPrimitiveCall> primitives{};
  ParameterList params{};
  ParameterList sub_params{};
  RegisterSync register_sync{};
  std::vector<Data> const_values{};
  // should I store the objects in the vector, instead of pointers?
  std::vector<std::unique_ptr<Expression> > expressions{};
  std::vector<std::string> strings{};
  size_t num_params;

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

  // log event, notify debugger if needed, lock registers, then call execute()
  void operator()(Packet *pkt) const;

  void execute(Packet *pkt) const;

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

  // NOLINTNEXTLINE(whitespace/operators)
  ActionFnEntry(ActionFnEntry &&other) /*noexcept*/ = default;
  // NOLINTNEXTLINE(whitespace/operators)
  ActionFnEntry &operator=(ActionFnEntry &&other) /*noexcept*/ = default;

 private:
  const ActionFn *action_fn{nullptr};
  ActionData action_data{};
};

}  // namespace bm

#endif  // BM_BM_SIM_ACTIONS_H_
