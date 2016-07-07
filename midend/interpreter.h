/*
Copyright 2016 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _MIDEND_INTERPRETER_H_
#define _MIDEND_INTERPRETER_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/coreLibrary.h"

// Symbolic P4 program evaluation.

namespace P4 {

class ValueFactory;

// Base class for all abstract values
class AbstractValue {
    static unsigned crtid;
 protected:
    virtual ~AbstractValue() {}
 public:
    const unsigned id;
    AbstractValue() : id(crtid++) {}
    virtual bool isScalar() const = 0;
    virtual void dbprint(std::ostream& out) const = 0;
    template<typename T> T* to() {
        CHECK_NULL(this);
        auto result = dynamic_cast<T*>(this);
        CHECK_NULL(result); return result; }
    template<typename T> const T* to() const {
        CHECK_NULL(this);
        auto result = dynamic_cast<const T*>(this);
        CHECK_NULL(result); return result; }
    template<typename T> bool is() const { return dynamic_cast<const T*>(this) != nullptr; }
    virtual AbstractValue* clone() const = 0;
    virtual void setAllUnknown() = 0;
    virtual void assign(const AbstractValue* other) = 0;
    virtual void merge(const AbstractValue* other) = 0;
};

// Creates values from type declarations
class ValueFactory {
    const TypeMap* typeMap;
 public:
    explicit ValueFactory(const TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); }
    AbstractValue* create(const IR::Type* type, bool uninitialized) const;
    // True if type has a fixed width, i.e., it does not contain a Varbit.
    bool isFixedWidth(const IR::Type* type) const;
    // If type has a fixed width return width in bits.
    // varbit types are assumed to have width 0 when counting.
    // Does not count the size for the "valid" bit for headers.
    unsigned getWidth(const IR::Type* type) const;
};

class ValueMap final {
    std::map<const IR::IDeclaration*, AbstractValue*> map;
 public:
    ValueMap* clone() const {
        auto result = new ValueMap();
        for (auto v : map)
            result->map.emplace(v.first, v.second->clone());
        return result;
    }
    void set(const IR::IDeclaration* left, AbstractValue* right)
    { CHECK_NULL(left); CHECK_NULL(right); map[left] = right; }
    AbstractValue* get(const IR::IDeclaration* left) const
    { CHECK_NULL(left); return ::get(map, left); }

    void dbprint(std::ostream& out) const {
        bool first = true;
        for (auto f : map) {
            if (!first)
                out << std::endl;
            out << f.first << "=>" << f.second;
            first = false;
        }
    }
    void merge(ValueMap* other) {
        BUG_CHECK(map.size() == other->map.size(), "Merging incompatible maps?");
        for (auto d : map) {
            auto v = other->get(d.first);
            d.second->merge(v);
        }
    }
};

class ExpressionEvaluator : public Inspector {
    const ReferenceMap* refMap;
    TypeMap*            typeMap;  // updated if constant folding happens
    ValueMap*           valueMap;
    const ValueFactory* factory;

    std::map<const IR::Expression*, AbstractValue*> value;

    AbstractValue* set(const IR::Expression* expression, AbstractValue* v)
    { value.emplace(expression, v); return v; }
    AbstractValue* get(const IR::Expression* expression) const {
        auto r = ::get(value, expression);
        BUG_CHECK(r != nullptr, "no evaluation for %1%", expression);
        return r;
    }
    void postorder(const IR::Constant* expression) override;
    void postorder(const IR::BoolLiteral* expression) override;
    void postorder(const IR::Operation_Binary* expression) override;
    void postorder(const IR::Operation_Relation* expression) override;
    void postorder(const IR::Operation_Unary* expression) override;
    void postorder(const IR::PathExpression* expression) override;
    void postorder(const IR::Member* expression) override;
    void postorder(const IR::ArrayIndex* expression) override;
    void postorder(const IR::ListExpression* expression) override;
    void postorder(const IR::MethodCallExpression* expression) override;

 public:
    ExpressionEvaluator(const ReferenceMap* refMap, TypeMap* typeMap, ValueMap* valueMap) :
            refMap(refMap), typeMap(typeMap), valueMap(valueMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(valueMap);
        factory = new ValueFactory(typeMap);
    }

    // May mutate the valueMap, when evaluating expression with side-effects
    AbstractValue* evaluate(const IR::Expression*);
};

//////////////////////////////////////////////////////////////////////////////////////////////

// produced when evaluation gives a static error
class ErrorValue : public AbstractValue {
 public:
    const IR::Node* errorPosition;
    explicit ErrorValue(const IR::Node* errorPosition) : errorPosition(errorPosition) {}
    void setAllUnknown() override {}
    void assign(const AbstractValue*) override {}
    bool isScalar() const override { return true; }
    void merge(const AbstractValue*) override
    { BUG("%1%: cannot merge errors", this); }
    virtual cstring message() const = 0;
};

class ExceptionValue : public ErrorValue {
 public:
    const P4::StandardExceptions exc;
    ExceptionValue(const IR::Node* errorPosition, P4::StandardExceptions exc) :
            ErrorValue(errorPosition), exc(exc) {}
    AbstractValue* clone() const override { return new ExceptionValue(errorPosition, exc); }
    void dbprint(std::ostream& out) const override
    { out << "Exception: " << exc; }
    cstring message() const override {
        std::stringstream str;
        str << exc;
        return str.str();
    }
};

class StaticErrorValue : public ErrorValue {
 public:
    const cstring msg;
    StaticErrorValue(const IR::Node* errorPosition, cstring message) :
            ErrorValue(errorPosition), msg(message) {}
    AbstractValue* clone() const override { return new StaticErrorValue(errorPosition, msg); }
    void dbprint(std::ostream& out) const override
    { out << "Error: " << msg; }
    cstring message() const override { return msg; }
};

class ScalarValue : public AbstractValue {
 public:
    enum class ValueState {
        Uninitialized,
        NotConstant,  // we cannot tell statically
        Constant  // compile-time constant
    };

 protected:
    explicit ScalarValue(ScalarValue::ValueState state) : state(state) {}

 public:
    ValueState state;
    bool isUninitialized() const { return state == ValueState::Uninitialized; }
    bool isUnknown() const { return state == ValueState::NotConstant; }
    bool isKnown() const { return state == ValueState::Constant; }
    bool isScalar() const override { return true; }
    void dbprint(std::ostream& out) const override {
        if (isUninitialized())
            out << "uninitialized";
        else if (isUnknown())
            out << "unknown";
    }
    static ValueState init(bool uninit)
    { return uninit ? ValueState::Uninitialized : ValueState::NotConstant; }
    void setAllUnknown() override
    { state = ScalarValue::ValueState::NotConstant; }
    ValueState mergeState(ValueState other) const {
        if (state == ValueState::Uninitialized && other == ValueState::Uninitialized)
            return ValueState::Uninitialized;
        if (state == ValueState::Constant && other == ValueState::Constant)
            // This may be wrong.
            return ValueState::Constant;
        return ValueState::NotConstant;
    }
};

class VoidValue : public AbstractValue {
    VoidValue() {}
    static VoidValue* instance;
 public:
    void dbprint(std::ostream& out) const override { out << "void"; }
    void setAllUnknown() override {}
    bool isScalar() const override { return false; }
    void assign(const AbstractValue*) override
    { BUG("assign to void"); }
    static VoidValue* get() { return instance; }
    AbstractValue* clone() const override { return instance; }
    void merge(const AbstractValue* other) override
    { BUG_CHECK(other->is<VoidValue>(), "%1%: expected void", other); }
};

class AbstractBool final : public ScalarValue {
 public:
    bool value;
    explicit AbstractBool(ScalarValue::ValueState state) : ScalarValue(state), value(false) {}
    AbstractBool() : ScalarValue(ScalarValue::ValueState::Uninitialized), value(false) {}
    explicit AbstractBool(const IR::BoolLiteral* constant) :
            ScalarValue(ScalarValue::ValueState::Constant), value(constant->value) {}
    AbstractBool(const AbstractBool& other) = default;
    explicit AbstractBool(bool value) :
            ScalarValue(ScalarValue::ValueState::Constant), value(value) {}
    void dbprint(std::ostream& out) const override {
        ScalarValue::dbprint(out);
        if (!isKnown()) return;
        out << (value ? "true" : "false"); }
    AbstractValue* clone() const override {
        auto result = new AbstractBool();
        result->state = state;
        result->value = value;
        return result;
    }
    void assign(const AbstractValue* other) override;
    void merge(const AbstractValue* other) override;
};

class AbstractInteger final : public ScalarValue {
 public:
    const IR::Constant* constant;
    AbstractInteger() : ScalarValue(ScalarValue::ValueState::Uninitialized), constant(nullptr) {}
    explicit AbstractInteger(ScalarValue::ValueState state) : ScalarValue(state) {}
    explicit AbstractInteger(const IR::Constant* constant) :
            ScalarValue(ScalarValue::ValueState::Constant), constant(constant)
    { CHECK_NULL(constant); }
    AbstractInteger(const AbstractInteger& other) = default;
    void dbprint(std::ostream& out) const override {
        ScalarValue::dbprint(out);
        if (!isKnown()) return;
        out << constant->value; }
    AbstractValue* clone() const override {
        auto result = new AbstractInteger();
        result->state = state;
        result->constant = constant;
        return result;
    }
    void assign(const AbstractValue* other) override;
    void merge(const AbstractValue* other) override;
};

class StructValue : public AbstractValue {
 protected:
    std::map<cstring, AbstractValue*> fieldValue;
    explicit StructValue(const IR::Type_StructLike* type) : type(type) { CHECK_NULL(type); }

 public:
    const IR::Type_StructLike* type;
    StructValue(const IR::Type_StructLike* type, bool uninitialized, const ValueFactory* factory);
    virtual AbstractValue* get(const IR::Node*, cstring field) const {
        auto r = ::get(fieldValue, field);
        CHECK_NULL(r);
        return r;
    }
    void set(cstring field, AbstractValue* value) {
        CHECK_NULL(value);
        fieldValue[field] = value;
    }
    void dbprint(std::ostream& out) const override {
        bool first = true;
        out << "{ ";
        for (auto f : fieldValue) {
            if (!first)
                out << ", ";
            out << f.first << "=>" << f.second;
            first = false;
        }
        out << " }";
    }
    bool isScalar() const override { return false; }
    AbstractValue* clone() const override;
    void setAllUnknown() override;
    void assign(const AbstractValue* other) override;
    void merge(const AbstractValue* other) override;
};

class HeaderValue : public StructValue {
 public:
    explicit HeaderValue(const IR::Type_Header* type) : StructValue(type) {}
    AbstractBool* valid;
    HeaderValue(const IR::Type_Header* type, bool uninitialized, const ValueFactory* factory);
    virtual void setValid(bool v);
    AbstractValue* clone() const override;
    AbstractValue* get(const IR::Node* node, cstring field) const override;
    void setAllUnknown() override;
    void assign(const AbstractValue* other) override;
    void dbprint(std::ostream& out) const override {
        out << "valid=>";
        valid->dbprint(out);
        out << " ";
        StructValue::dbprint(out);
    }
    void merge(const AbstractValue* other) override;
};

class ArrayValue final : public AbstractValue {
    std::vector<HeaderValue*> values;
    friend class AnyElement;
    explicit ArrayValue(const IR::Type_Header* elemType) : elemType(elemType) {}

 public:
    const IR::Type_Header* elemType;
    ArrayValue(const IR::Type_Header* elemType, size_t size,
               bool uninitialized, const ValueFactory* factory);
    AbstractValue* get(const IR::Node* node, size_t index) const {
        if (index >= values.size())
            return new StaticErrorValue(node, "Out of bounds");
        return values.at(index);
    }
    void shift(int amount);
    void set(size_t index, HeaderValue* value) {
        CHECK_NULL(value);
        values[index] = value;
    }
    void dbprint(std::ostream& out) const override {
        bool first = true;
        for (auto f : values) {
            if (!first)
                out << ", ";
            out << f;
            first = false;
        }
    }
    AbstractValue* clone() const override;
    AbstractValue* next(const IR::Node* node);
    AbstractValue* last(const IR::Node* node);
    bool isScalar() const override { return false; }
    void setAllUnknown() override;
    void assign(const AbstractValue* other) override;
    void merge(const AbstractValue* other) override;
};

// Represents any element from a stack
class AnyElement final : public HeaderValue {
    ArrayValue* parent;
 public:
    explicit AnyElement(ArrayValue* parent) : HeaderValue(parent->elemType), parent(parent)
    { CHECK_NULL(parent); valid = new AbstractBool(); }
    AbstractValue* clone() const override
    { auto result = new AnyElement(parent); return result; }
    void setAllUnknown() override
    { parent->setAllUnknown(); }
    void assign(const AbstractValue*) override
    { parent->setAllUnknown(); }
    void dbprint(std::ostream& out) const override
    { out << "Any element of " << parent; }
    void setValid(bool) override { parent->setAllUnknown(); }
    void merge(const AbstractValue* other) override;
};

class TupleValue final : public AbstractValue {
    std::vector<AbstractValue*> values;
 public:
    TupleValue() = default;
    TupleValue(const IR::Type_Tuple* type,
               bool uninitialized, const ValueFactory* factory);
    AbstractValue* get(size_t index) const { return values.at(index); }
    void dbprint(std::ostream& out) const override {
        bool first = true;
        for (auto f : values) {
            if (!first)
                out << ", ";
            out << f;
            first = false;
        }
    }
    AbstractValue* clone() const override;
    bool isScalar() const override { return false; }
    void setAllUnknown() override;
    void assign(const AbstractValue*) override
    { BUG("%1%: tuples are read-only", this); }
    void add(AbstractValue* value)
    { values.push_back(value); }
    void merge(const AbstractValue* other) override;
};

class StateValue final : public AbstractValue {
    const IR::ParserState* state;
 public:
    explicit StateValue(const IR::ParserState* state) : state(state) {}
    AbstractValue* clone() const override
    { return new StateValue(state); }
    bool isScalar() const override { return true; }
    void setAllUnknown() override
    { BUG("%1%: states are read-only", this); }
    void assign(const AbstractValue*) override
    { BUG("%1%: states are read-only", this); }
    void dbprint(std::ostream& out) const override
    { out << state->name; }
    void merge(const AbstractValue*) override
    { BUG("merging states are read-only"); }
};

// Some extern value of an unknown type
class ExternValue : public AbstractValue {
 protected:
    const IR::Type_Extern* type;
 public:
    explicit ExternValue(const IR::Type_Extern* type) : type(type)
    { CHECK_NULL(type); }
    void dbprint(std::ostream& out) const override
    { out << "instance of " << type; }
    AbstractValue* clone() const override
    { return new ExternValue(type); }
    bool isScalar() const override { return false; }
    void setAllUnknown() override
    { BUG("%1%: extern is read-only", this); }
    void assign(const AbstractValue*) override
    { BUG("%1%: extern is read-only", this); }
    void merge(const AbstractValue*) override {}
};

// Models an extern of type packet_in
class PacketInValue final : public ExternValue {
    // Minimum offset in the stream.
    // Extracting to a varbit may advance the stream offset
    // by an unknown quantity.  Varbits are counted as 0
    // (as per ValueFactory::getWidth).
    unsigned minimumStreamOffset;
 public:
    explicit PacketInValue(const IR::Type_Extern* type) : ExternValue(type) {}
    void dbprint(std::ostream& out) const override
    { out << "packet_in; offset =" << minimumStreamOffset; }
    AbstractValue* clone() const override {
        auto result = new PacketInValue(type);
        result->minimumStreamOffset = minimumStreamOffset;
        return result;
    }
    void advance(const IR::Type* type);
    void merge(const AbstractValue* other) override;
};

}  // namespace P4

#endif /* _MIDEND_INTERPRETER_H_ */
