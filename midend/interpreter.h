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

// A form of abstract interpretation where we try to evaluate as much of the program as we can
// statically.

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
};

// produced when evaluation gives a static error
class ErrorValue : public AbstractValue {
 public:
    cstring message;
    explicit ErrorValue(cstring message) : message(message) {}
    AbstractValue* clone() const override { return new ErrorValue(message); }
    void setAllUnknown() override {}
    void assign(const AbstractValue*) override {}
    bool isScalar() const override { return true; }
    void dbprint(std::ostream& out) const override
    { out << "Error: " << message; }
};

// Creates values from type declarations
class ValueFactory {
    const TypeMap* typeMap;
    // TODO: should be able to initialize values
 public:
    explicit ValueFactory(const TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); }
    AbstractValue* create(const IR::Type* type, bool uninitialized) const;
};

class ScalarValue : public AbstractValue {
 public:
    enum class ValueState {
        Uninitialized,
        NotConstant,
        Constant
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
};

class StructValue : public AbstractValue {
 protected:
    std::map<cstring, AbstractValue*> fieldValue;
    explicit StructValue(const IR::Type_StructLike* type) : type(type) { CHECK_NULL(type); }

 public:
    const IR::Type_StructLike* type;
    StructValue(const IR::Type_StructLike* type, bool uninitialized, const ValueFactory* factory);
    virtual AbstractValue* get(cstring field) const {
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
};

class HeaderValue : public StructValue {
 public:
    explicit HeaderValue(const IR::Type_Header* type) : StructValue(type) {}
    AbstractBool* valid;
    HeaderValue(const IR::Type_Header* type, bool uninitialized, const ValueFactory* factory);
    virtual void setValid(bool v);
    AbstractValue* clone() const override;
    void setAllUnknown() override;
    void assign(const AbstractValue* other) override;
    void dbprint(std::ostream& out) const override {
        out << "valid=>";
        valid->dbprint(out);
        out << " ";
        StructValue::dbprint(out);
    }
};

class ArrayValue final : public AbstractValue {
    std::vector<HeaderValue*> values;
    explicit ArrayValue(const IR::Type_Header* elemType) : elemType(elemType) {}

 public:
    const IR::Type_Header* elemType;
    ArrayValue(const IR::Type_Header* elemType, size_t size,
               bool uninitialized, const ValueFactory* factory);
    HeaderValue* get(size_t index) const { return values.at(index); }
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
    AbstractValue* next();
    AbstractValue* last();
    bool isScalar() const override { return false; }
    void setAllUnknown() override;
    void assign(const AbstractValue* other) override;
};

// Represents any element from a stack
class AnyElement final : public HeaderValue {
    ArrayValue* parent;
 public:
    explicit AnyElement(ArrayValue* parent) : HeaderValue(parent->elemType), parent(parent)
    { CHECK_NULL(parent); valid = new AbstractBool(); }
    AbstractValue* clone() const override
    { auto result = parent->get(0)->clone(); result->setAllUnknown(); return result; }
    void setAllUnknown() override
    { parent->setAllUnknown(); }
    void assign(const AbstractValue*) override
    { parent->setAllUnknown(); }
    void dbprint(std::ostream& out) const override
    { out << "Any element of " << parent; }
    void setValid(bool) override { parent->setAllUnknown(); }
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
};

class ExpressionEvaluator : public Inspector {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    ValueMap*     valueMap;
    ValueFactory* factory;

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
    void postorder(const IR::MethodCallExpression* expression) override;

 public:
    ExpressionEvaluator(ReferenceMap* refMap, TypeMap* typeMap, ValueMap* valueMap) :
            refMap(refMap), typeMap(typeMap), valueMap(valueMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(valueMap);
        factory = new ValueFactory(typeMap);
    }

    // May mutate the valueMap, when evaluating expression with side-effects
    AbstractValue* evaluate(const IR::Expression*);
};

}  // namespace P4

#endif /* _MIDEND_INTERPRETER_H_ */
