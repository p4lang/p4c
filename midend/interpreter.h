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

class SymbolicValueFactory;

// Base class for all abstract values
class SymbolicValue {
    static unsigned crtid;

 protected:
    explicit SymbolicValue(const IR::Type* type) : id(crtid++), type(type) {}

 public:
    const unsigned id;
    const IR::Type* type;
    virtual bool isScalar() const = 0;
    virtual void dbprint(std::ostream& out) const = 0;
    template<typename T> T* to() {
        auto result = dynamic_cast<T*>(this);
        CHECK_NULL(result); return result; }
    template<typename T> const T* to() const {
        auto result = dynamic_cast<const T*>(this);
        CHECK_NULL(result); return result; }
    template<typename T> bool is() const { return dynamic_cast<const T*>(this) != nullptr; }
    virtual SymbolicValue* clone() const = 0;
    virtual void setAllUnknown() = 0;
    virtual void assign(const SymbolicValue* other) = 0;
    // Merging two symbolic values; values should form a lattice.
    // Returns 'true' if merging changed the current value.
    virtual bool merge(const SymbolicValue* other) = 0;
    virtual bool equals(const SymbolicValue* other) const = 0;
    // True if some parts of this value are definitely uninitialized
    virtual bool hasUninitializedParts() const = 0;
};

// Creates values from type declarations
class SymbolicValueFactory {
    const TypeMap* typeMap;
 public:
    explicit SymbolicValueFactory(const TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); }
    SymbolicValue* create(const IR::Type* type, bool uninitialized) const;
    // True if type has a fixed width, i.e., it does not contain a Varbit.
    bool isFixedWidth(const IR::Type* type) const;
    // If type has a fixed width return width in bits.
    // varbit types are assumed to have width 0 when counting.
    // Does not count the size for the "valid" bit for headers.
    unsigned getWidth(const IR::Type* type) const;
};

class ValueMap final : public IHasDbPrint {
 public:
    std::map<const IR::IDeclaration*, SymbolicValue*> map;
    ValueMap* clone() const {
        auto result = new ValueMap();
        for (auto v : map)
            result->map.emplace(v.first, v.second->clone());
        return result;
    }
    ValueMap* filter(std::function<bool(const IR::IDeclaration*, const SymbolicValue*)> filter) {
        auto result = new ValueMap();
        for (auto v : map)
            if (filter(v.first, v.second))
                result->map.emplace(v.first, v.second);
        return result;
    }
    void set(const IR::IDeclaration* left, SymbolicValue* right)
    { CHECK_NULL(left); CHECK_NULL(right); map[left] = right; }
    SymbolicValue* get(const IR::IDeclaration* left) const
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
    bool merge(const ValueMap* other) {
        bool change = false;
        BUG_CHECK(map.size() == other->map.size(), "Merging incompatible maps?");
        for (auto d : map) {
            auto v = other->get(d.first);
            CHECK_NULL(v);
            change = change || d.second->merge(v);
        }
        return change;
    }
    bool equals(const ValueMap* other) const {
        BUG_CHECK(map.size() == other->map.size(), "Incompatible maps compared");
        for (auto v : map) {
            auto ov = other->get(v.first);
            CHECK_NULL(ov);
            if (!v.second->equals(ov))
                return false;
        }
        return true;
    }
};

class ExpressionEvaluator : public Inspector {
    ReferenceMap*       refMap;
    TypeMap*            typeMap;  // updated if constant folding happens
    ValueMap*           valueMap;
    const SymbolicValueFactory* factory;
    bool evaluatingLeftValue = false;

    std::map<const IR::Expression*, SymbolicValue*> value;

    SymbolicValue* set(const IR::Expression* expression, SymbolicValue* v)
    { value.emplace(expression, v); return v; }
    SymbolicValue* get(const IR::Expression* expression) const {
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
    bool preorder(const IR::ArrayIndex* expression) override;
    void postorder(const IR::ArrayIndex* expression) override;
    void postorder(const IR::ListExpression* expression) override;
    void postorder(const IR::MethodCallExpression* expression) override;

 public:
    ExpressionEvaluator(ReferenceMap* refMap, TypeMap* typeMap, ValueMap* valueMap) :
            refMap(refMap), typeMap(typeMap), valueMap(valueMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(valueMap);
        factory = new SymbolicValueFactory(typeMap);
    }

    // May mutate the valueMap, when evaluating expression with side-effects.
    // If leftValue is true we are returning a leftValue.
    SymbolicValue* evaluate(const IR::Expression* expression, bool leftValue);
};

//////////////////////////////////////////////////////////////////////////////////////////////

// produced when evaluation gives a static error
class SymbolicError : public SymbolicValue {
 public:
    const IR::Node* errorPosition;
    explicit SymbolicError(const IR::Node* errorPosition) :
            SymbolicValue(nullptr), errorPosition(errorPosition) {}
    void setAllUnknown() override {}
    void assign(const SymbolicValue*) override {}
    bool isScalar() const override { return true; }
    bool merge(const SymbolicValue*) override
    { BUG("%1%: cannot merge errors", this); return false; }
    virtual cstring message() const = 0;
    bool hasUninitializedParts() const override
    { return false; }
};

class SymbolicException : public SymbolicError {
 public:
    const P4::StandardExceptions exc;
    SymbolicException(const IR::Node* errorPosition, P4::StandardExceptions exc) :
            SymbolicError(errorPosition), exc(exc) {}
    SymbolicValue* clone() const override { return new SymbolicException(errorPosition, exc); }
    void dbprint(std::ostream& out) const override
    { out << "Exception: " << exc; }
    cstring message() const override {
        std::stringstream str;
        str << exc;
        return str.str();
    }
    bool equals(const SymbolicValue* other) const override;
};

class SymbolicStaticError : public SymbolicError {
 public:
    const cstring msg;
    SymbolicStaticError(const IR::Node* errorPosition, cstring message) :
            SymbolicError(errorPosition), msg(message) {}
    SymbolicValue* clone() const override { return new SymbolicStaticError(errorPosition, msg); }
    void dbprint(std::ostream& out) const override
    { out << "Error: " << msg; }
    cstring message() const override { return msg; }
    bool equals(const SymbolicValue* other) const override;
};

class ScalarValue : public SymbolicValue {
 public:
    enum class ValueState {
        Uninitialized,
        NotConstant,  // we cannot tell statically
        Constant  // compile-time constant
    };

 protected:
    ScalarValue(ScalarValue::ValueState state, const IR::Type* type) :
            SymbolicValue(type), state(state) {}

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
    bool hasUninitializedParts() const override
    { return state == ValueState::Uninitialized; }
};

class SymbolicVoid : public SymbolicValue {
    SymbolicVoid() : SymbolicValue(IR::Type_Void::get()) {}
    static SymbolicVoid* instance;
 public:
    void dbprint(std::ostream& out) const override { out << "void"; }
    void setAllUnknown() override {}
    bool isScalar() const override { return false; }
    void assign(const SymbolicValue*) override
    { BUG("assign to void"); }
    static SymbolicVoid* get() { return instance; }
    SymbolicValue* clone() const override { return instance; }
    bool merge(const SymbolicValue* other) override
    { BUG_CHECK(other->is<SymbolicVoid>(), "%1%: expected void", other); return false; }
    bool equals(const SymbolicValue* other) const override
    { return other == instance; }
    bool hasUninitializedParts() const override
    { return false; }
};

class SymbolicBool final : public ScalarValue {
 public:
    bool value;
    explicit SymbolicBool(ScalarValue::ValueState state) :
            ScalarValue(state, IR::Type_Boolean::get()), value(false) {}
    SymbolicBool() : ScalarValue(ScalarValue::ValueState::Uninitialized, IR::Type_Boolean::get()),
            value(false) {}
    explicit SymbolicBool(const IR::BoolLiteral* constant) :
            ScalarValue(ScalarValue::ValueState::Constant, IR::Type_Boolean::get()),
            value(constant->value) {}
    SymbolicBool(const SymbolicBool& other) = default;
    explicit SymbolicBool(bool value) :
            ScalarValue(ScalarValue::ValueState::Constant, IR::Type_Boolean::get()), value(value) {}
    void dbprint(std::ostream& out) const override {
        ScalarValue::dbprint(out);
        if (!isKnown()) return;
        out << (value ? "true" : "false"); }
    SymbolicValue* clone() const override {
        auto result = new SymbolicBool();
        result->state = state;
        result->value = value;
        return result;
    }
    void assign(const SymbolicValue* other) override;
    bool merge(const SymbolicValue* other) override;
    bool equals(const SymbolicValue* other) const override;
};

class SymbolicInteger final : public ScalarValue {
 public:
    const IR::Constant* constant;
    explicit SymbolicInteger(const IR::Type_Bits* type) :
            ScalarValue(ScalarValue::ValueState::Uninitialized, type), constant(nullptr) {}
    SymbolicInteger(ScalarValue::ValueState state, const IR::Type_Bits* type) :
            ScalarValue(state, type) {}
    explicit SymbolicInteger(const IR::Constant* constant) :
            ScalarValue(ScalarValue::ValueState::Constant, constant->type), constant(constant)
    { CHECK_NULL(constant); }
    SymbolicInteger(const SymbolicInteger& other) = default;
    void dbprint(std::ostream& out) const override {
        ScalarValue::dbprint(out);
        if (isKnown())
            out << constant->value; }
    SymbolicValue* clone() const override {
        auto result = new SymbolicInteger(type->to<IR::Type_Bits>());
        result->state = state;
        result->constant = constant;
        return result;
    }
    void assign(const SymbolicValue* other) override;
    bool merge(const SymbolicValue* other) override;
    bool equals(const SymbolicValue* other) const override;
};

class SymbolicVarbit final : public ScalarValue {
 public:
    explicit SymbolicVarbit(const IR::Type_Varbits* type) :
            ScalarValue(ScalarValue::ValueState::Uninitialized, type) {}
    SymbolicVarbit(ScalarValue::ValueState state, const IR::Type_Varbits* type) :
            ScalarValue(state, type) {}
    SymbolicVarbit(const SymbolicVarbit& other) = default;
    void dbprint(std::ostream& out) const override
    { ScalarValue::dbprint(out); }
    SymbolicValue* clone() const override
    { return new SymbolicVarbit(state, type->to<IR::Type_Varbits>()); }
    void assign(const SymbolicValue* other) override;
    bool merge(const SymbolicValue* other) override;
    bool equals(const SymbolicValue* other) const override;
};

// represents enum, error, and match_kind
class SymbolicEnum final : public ScalarValue {
    IR::ID value;
 public:
    explicit SymbolicEnum(const IR::Type* type) :
            ScalarValue(ScalarValue::ValueState::Uninitialized, type) {}
    SymbolicEnum(ScalarValue::ValueState state, const IR::Type* type, const IR::ID value) :
            ScalarValue(state, type), value(value) {}
    SymbolicEnum(const IR::Type* type, const IR::ID value) :
            ScalarValue(ScalarValue::ValueState::Constant, type), value(value) {}
    SymbolicEnum(const SymbolicEnum& other) = default;
    void dbprint(std::ostream& out) const override {
        ScalarValue::dbprint(out);
        if (isKnown())
            out << value; }
    SymbolicValue* clone() const override
    { return new SymbolicEnum(state, type, value); }
    void assign(const SymbolicValue* other) override;
    bool merge(const SymbolicValue* other) override;
    bool equals(const SymbolicValue* other) const override;
};

class SymbolicStruct : public SymbolicValue {
 protected:
    explicit SymbolicStruct(const IR::Type_StructLike* type) :
            SymbolicValue(type) { CHECK_NULL(type); }

 public:
    std::map<cstring, SymbolicValue*> fieldValue;
    SymbolicStruct(const IR::Type_StructLike* type, bool uninitialized,
                   const SymbolicValueFactory* factory);
    virtual SymbolicValue* get(const IR::Node*, cstring field) const {
        auto r = ::get(fieldValue, field);
        CHECK_NULL(r);
        return r;
    }
    void set(cstring field, SymbolicValue* value) {
        CHECK_NULL(value);
        fieldValue[field] = value;
    }
    void dbprint(std::ostream& out) const override;
    bool isScalar() const override { return false; }
    SymbolicValue* clone() const override;
    void setAllUnknown() override;
    void assign(const SymbolicValue* other) override;
    bool merge(const SymbolicValue* other) override;
    bool equals(const SymbolicValue* other) const override;
    bool hasUninitializedParts() const override;
};

class SymbolicHeader : public SymbolicStruct {
 public:
    explicit SymbolicHeader(const IR::Type_Header* type) : SymbolicStruct(type) {}
    SymbolicBool* valid;
    SymbolicHeader(const IR::Type_Header* type, bool uninitialized,
                   const SymbolicValueFactory* factory);
    virtual void setValid(bool v);
    SymbolicValue* clone() const override;
    SymbolicValue* get(const IR::Node* node, cstring field) const override;
    void setAllUnknown() override;
    void assign(const SymbolicValue* other) override;
    void dbprint(std::ostream& out) const override;
    bool merge(const SymbolicValue* other) override;
    bool equals(const SymbolicValue* other) const override;
};

class SymbolicArray final : public SymbolicValue {
    std::vector<SymbolicHeader*> values;
    friend class AnyElement;
    explicit SymbolicArray(const IR::Type_Stack* type) :
            SymbolicValue(type), size(type->getSize()),
            elemType(type->elementType->to<IR::Type_Header>()) {}

 public:
    const size_t size;
    const IR::Type_Header* elemType;
    SymbolicArray(const IR::Type_Stack* stack, bool uninitialized,
                  const SymbolicValueFactory* factory);
    SymbolicValue* get(const IR::Node* node, size_t index) const {
        if (index >= values.size())
            return new SymbolicStaticError(node, "Out of bounds");
        return values.at(index);
    }
    void shift(int amount);  // negative = shift left
    void set(size_t index, SymbolicHeader* value) {
        CHECK_NULL(value);
        values[index] = value;
    }
    void dbprint(std::ostream& out) const override;
    SymbolicValue* clone() const override;
    SymbolicValue* next(const IR::Node* node);
    SymbolicValue* last(const IR::Node* node);
    bool isScalar() const override { return false; }
    void setAllUnknown() override;
    void assign(const SymbolicValue* other) override;
    bool merge(const SymbolicValue* other) override;
    bool equals(const SymbolicValue* other) const override;
    bool hasUninitializedParts() const override;
};

// Represents any element from a stack
class AnyElement final : public SymbolicHeader {
    SymbolicArray* parent;
 public:
    explicit AnyElement(SymbolicArray* parent) :
            SymbolicHeader(parent->elemType), parent(parent)
    { CHECK_NULL(parent); valid = new SymbolicBool(); }
    SymbolicValue* clone() const override
    { auto result = new AnyElement(parent); return result; }
    void setAllUnknown() override
    { parent->setAllUnknown(); }
    void assign(const SymbolicValue*) override
    { parent->setAllUnknown(); }
    void dbprint(std::ostream& out) const override
    { out << "Any element of " << parent; }
    void setValid(bool) override { parent->setAllUnknown(); }
    bool merge(const SymbolicValue* other) override;
    bool equals(const SymbolicValue* other) const override;
    SymbolicValue* collapse() const;
    bool hasUninitializedParts() const override
    { BUG("Should not be called"); }
};

class SymbolicTuple final : public SymbolicValue {
    std::vector<SymbolicValue*> values;

 public:
    explicit SymbolicTuple(const IR::Type_Tuple* type) :
            SymbolicValue(type) {}
    SymbolicTuple(const IR::Type_Tuple* type,
               bool uninitialized, const SymbolicValueFactory* factory);
    SymbolicValue* get(size_t index) const { return values.at(index); }
    void dbprint(std::ostream& out) const override {
        bool first = true;
        for (auto f : values) {
            if (!first)
                out << ", ";
            out << f;
            first = false;
        }
    }
    SymbolicValue* clone() const override;
    bool isScalar() const override { return false; }
    void setAllUnknown() override;
    void assign(const SymbolicValue*) override
    { BUG("%1%: tuples are read-only", this); }
    void add(SymbolicValue* value)
    { values.push_back(value); }
    bool merge(const SymbolicValue* other) override;
    bool equals(const SymbolicValue* other) const override;
    bool hasUninitializedParts() const override;
};

// Some extern value of an unknown type
class SymbolicExtern : public SymbolicValue {
 public:
    explicit SymbolicExtern(const IR::Type_Extern* type) :
            SymbolicValue(type)
    { CHECK_NULL(type); }
    void dbprint(std::ostream& out) const override
    { out << "instance of " << type; }
    SymbolicValue* clone() const override
    { return new SymbolicExtern(type->to<IR::Type_Extern>()); }
    bool isScalar() const override { return false; }
    void setAllUnknown() override
    { BUG("%1%: extern is read-only", this); }
    void assign(const SymbolicValue*) override
    { BUG("%1%: extern is read-only", this); }
    bool merge(const SymbolicValue*) override { return false; }
    bool equals(const SymbolicValue* other) const override;
    bool hasUninitializedParts() const override
    { return false; }
};

// Models an extern of type packet_in
class SymbolicPacketIn final : public SymbolicExtern {
    // Minimum offset in the stream.
    // Extracting to a varbit may advance the stream offset
    // by an unknown quantity.  Varbits are counted as 0
    // (as per SymbolicValueFactory::getWidth).
    unsigned minimumStreamOffset;
    // If true the minimumStreamOffset is a conservative
    // approximation.
    bool conservative;

 public:
    explicit SymbolicPacketIn(const IR::Type_Extern* type) :
            SymbolicExtern(type), minimumStreamOffset(0), conservative(false) {}
    void dbprint(std::ostream& out) const override {
        out << "packet_in; offset =" << minimumStreamOffset <<
                (conservative ? " (conservative)" : ""); }
    SymbolicValue* clone() const override {
        auto result = new SymbolicPacketIn(type->to<IR::Type_Extern>());
        result->minimumStreamOffset = minimumStreamOffset;
        result->conservative = conservative;
        return result;
    }
    void setConservative()
    { conservative = true; }
    bool isConservative() const
    { return conservative; }
    void advance(unsigned width)
    { minimumStreamOffset += width; }
    bool merge(const SymbolicValue* other) override;
    bool equals(const SymbolicValue* other) const override;
};

}  // namespace P4

#endif /* _MIDEND_INTERPRETER_H_ */
