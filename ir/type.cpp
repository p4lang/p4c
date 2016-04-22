#include "ir.h"

#include "substitution.h"
#include "substitutionVisitor.h"

namespace IR {

const cstring IR::Type_Stack::next = "next";
const cstring IR::Type_Stack::last = "last";
const cstring IR::Type_Stack::full = "full";
const cstring IR::Type_Stack::empty = "empty";
const cstring IR::Type_Stack::push_front = "push_front";
const cstring IR::Type_Stack::pop_front = "pop_front";
const cstring IR::Type_Header::isValid = "isValid";
const cstring IR::Type_Header::setValid = "setValid";

const IR::ID IR::Type_Table::hit = ID("hit");
const IR::ID IR::Type_Table::action_run = ID("action_run");

const cstring IR::Annotation::nameAnnotation = "name";

int Type_Declaration::nextId = 0;
int Type_InfInt::nextId = 0;

Annotations* Annotations::empty = new Annotations(new Vector<Annotation>());

const Type::Unknown *Type::Unknown::get() {
    static const Type::Unknown *singleton;
    if (!singleton)
        singleton = (new Type::Unknown(Util::SourceInfo()));
    return singleton;
}

StructField::StructField(Util::SourceInfo si, ID name, const Type *type)
        : StructField(si, name, Annotations::empty, type) {}

StructField::StructField(ID name, const Type *type)
        : StructField(Util::SourceInfo(), name, Annotations::empty, type) {}

const Type::Boolean *Type::Boolean::get() {
    static const Type::Boolean *singleton;
    if (!singleton)
        singleton = (new Type::Boolean(Util::SourceInfo()));
    return singleton;
}

const Type::Bits *Type::Bits::get(Util::SourceInfo si, int sz, bool isSigned) {
    if (sz <= 0)
        ::error("%1%: Width cannot be negative or zero", si);
    return new Type::Bits(si, sz, isSigned);
}

const Type::Varbits *Type::Varbits::get(Util::SourceInfo si, int sz) {
    if (sz <= 0)
        ::error("%1%: Width cannot be negative or zero", si);
    return new Type::Varbits(si, sz);
}

const Type::Varbits *Type::Varbits::get() {
    return new Type::Varbits(Util::SourceInfo(), 0);
}

const Type_Dontcare *Type_Dontcare::get() {
    static const Type_Dontcare *singleton;
    if (!singleton)
        singleton = (new Type_Dontcare(Util::SourceInfo()));
    return singleton;
}

const Type_State *Type_State::get() {
    static const Type_State *singleton;
    if (!singleton)
        singleton = (new Type_State(Util::SourceInfo()));
    return singleton;
}

const Type_Void *Type_Void::get() {
    static const Type_Void *singleton;
    if (!singleton)
        singleton = (new Type_Void(Util::SourceInfo()));
    return singleton;
}

const Type_Error *Type_Error::get() {
    static const Type_Error *singleton;
    if (!singleton)
        singleton = (new Type_Error(Util::SourceInfo()));
    return singleton;
}

const Type_MatchKind *Type_MatchKind::get() {
    static const Type_MatchKind *singleton;
    if (!singleton)
        singleton = (new Type_MatchKind(Util::SourceInfo()));
    return singleton;
}

Type_Struct::Type_Struct(cstring name, const IR::NameMap<IR::StructField, ordered_map> &&fields)
        : Type_Struct(Util::SourceInfo(), name, IR::Annotations::empty, fields) {}

/**
 * Bind the parameters with the specified arguments.
 * @param arguments      Arguments to bind to the type's typeParameters.
 * @return               A type produced by specializing this generic type.
 * For example, given a type
 *
 * void _<T>(T data)
 *
 * it can be specialized to
 *
 * void _<int<32>>(int<32> data);
 */
const IR::Type* IMayBeGenericType::specialize(const IR::Vector<IR::Type>* arguments) const {
    TypeVariableSubstitution* bindings = new TypeVariableSubstitution();
    bool success = bindings->setBindings(getNode(), getTypeParameters(), arguments);
    if (!success)
        return nullptr;

    LOG1("Translation map\n" << bindings);

    TypeVariableSubstitutionVisitor tsv(bindings);
    const IR::Node* result = getNode()->apply(tsv);
    if (result == nullptr)
        return nullptr;

    LOG1("Specialized " << this << "\n\tinto " << result);
    return result->to<IR::Type>();
}

bool Type_ActionEnum::contains(cstring name) const {
    for (auto a : *actionList->actionList) {
        if (a->getName() == name)
            return true;
    }
    return false;
}

}  // namespace IR
