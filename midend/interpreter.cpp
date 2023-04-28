#include "interpreter.h"

#include "frontends/common/constantFolding.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
#include "lib/exceptions.h"

namespace P4 {

unsigned SymbolicValue::crtid = 0;

SymbolicValue *SymbolicValueFactory::create(const IR::Type *type, bool uninitialized) const {
    type = typeMap->getTypeType(type, true);
    if (type->is<IR::Type_Bits>())
        return new SymbolicInteger(ScalarValue::init(uninitialized), type->to<IR::Type_Bits>());
    if (type->is<IR::Type_Boolean>()) return new SymbolicBool(ScalarValue::init(uninitialized));
    if (type->is<IR::Type_Struct>())
        return new SymbolicStruct(type->to<IR::Type_Struct>(), uninitialized, this);
    if (type->is<IR::Type_Header>())
        return new SymbolicHeader(type->to<IR::Type_Header>(), uninitialized, this);
    if (type->is<IR::Type_HeaderUnion>())
        return new SymbolicHeaderUnion(type->to<IR::Type_HeaderUnion>(), uninitialized, this);
    if (type->is<IR::Type_Varbits>()) return new SymbolicVarbit(type->to<IR::Type_Varbits>());
    if (type->is<IR::Type_Stack>()) {
        auto st = type->to<IR::Type_Stack>();
        return new SymbolicArray(st, uninitialized, this);
    }
    if (type->is<IR::Type_Tuple>()) {
        auto tt = type->to<IR::Type_Tuple>();
        return new SymbolicTuple(tt, uninitialized, this);
    }
    if (type->is<IR::Type_Extern>()) {
        auto te = type->to<IR::Type_Extern>();
        if (te->name.name == P4CoreLibrary::instance().packetIn.name) {
            return new SymbolicPacketIn(te);
        }
        return new SymbolicExtern(te);
    }
    if (type->is<IR::Type_Enum>() || type->is<IR::Type_Error>() || type->is<IR::Type_MatchKind>()) {
        return new SymbolicEnum(type);
    }
    if (type->is<IR::Type_SpecializedCanonical>()) {
        auto spec = type->to<IR::Type_SpecializedCanonical>();
        return create(spec->substituted, uninitialized);
    }
    if (type->is<IR::Type_Parser>() || type->is<IR::Type_Control>() || type->is<IR::P4Parser>() ||
        type->is<IR::P4Control>())
        // This implies that inlining has not been done;
        // just ignore these values.
        return nullptr;

    BUG("%1%: unexpected type", type);
}

bool SymbolicValueFactory::isFixedWidth(const IR::Type *type) const {
    type = typeMap->getTypeType(type, true);
    if (type->is<IR::Type_Varbits>()) return false;
    if (type->is<IR::Type_Extern>()) return false;
    if (type->is<IR::Type_Stack>()) return isFixedWidth(type->to<IR::Type_Stack>()->elementType);
    if (type->is<IR::Type_StructLike>()) {
        auto st = type->to<IR::Type_StructLike>();
        for (auto f : st->fields)
            if (!isFixedWidth(f->type)) return false;
        return true;
    }
    if (type->is<IR::Type_Tuple>()) {
        auto tt = type->to<IR::Type_Tuple>();
        for (auto f : tt->components) {
            if (!isFixedWidth(f)) return false;
        }
        return true;
    }
    return true;
}

unsigned SymbolicValueFactory::getWidth(const IR::Type *type) const {
    type = typeMap->getTypeType(type, true);
    if (type->is<IR::Type_Bits>()) return type->to<IR::Type_Bits>()->size;
    if (type->is<IR::Type_Boolean>()) return 1;
    if (type->is<IR::Type_HeaderUnion>()) {
        unsigned width = 0;
        for (auto f : type->to<IR::Type_HeaderUnion>()->fields)
            width = std::max(width, getWidth(f->type));
        return width;
    }
    if (type->is<IR::Type_StructLike>()) {
        unsigned width = 0;
        auto st = type->to<IR::Type_StructLike>();
        for (auto f : st->fields) width += getWidth(f->type);
        return width;
    }
    if (type->is<IR::Type_Stack>()) {
        auto st = type->to<IR::Type_Stack>();
        auto bw = getWidth(st->elementType);
        return bw * st->getSize();
    }
    if (type->is<IR::Type_Tuple>()) {
        auto tt = type->to<IR::Type_Tuple>();
        unsigned width = 0;
        for (auto f : tt->components) width += getWidth(f);
        return width;
    }
    return 0;
}

/*********************************************************************************/

bool SymbolicException::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicException>()) return false;
    return (other->to<SymbolicException>()->exc == exc);
}

bool SymbolicStaticError::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicStaticError>()) return false;
    return msg == other->to<SymbolicStaticError>()->msg;
}

bool SymbolicBool::merge(const SymbolicValue *other) {
    auto bo = other->to<SymbolicBool>();
    auto saveState = state;
    state = mergeState(other->to<ScalarValue>()->state);
    if (state == ValueState::Constant && value != bo->value) state = ValueState::NotConstant;
    return (state != saveState);
}

void SymbolicBool::assign(const SymbolicValue *other) {
    BUG_CHECK(other->is<SymbolicBool>(), "%1%: expected a bool", other);
    auto bo = other->to<SymbolicBool>();
    state = bo->state;
    value = bo->value;
}

bool SymbolicBool::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicBool>()) return false;
    auto ab = other->to<SymbolicBool>();
    if (state != ab->state) return false;
    if (isKnown()) return value == ab->value;
    return true;
}

bool SymbolicInteger::merge(const SymbolicValue *other) {
    BUG_CHECK(other->is<SymbolicInteger>(), "%1%: expected an integer", other);
    auto io = other->to<SymbolicInteger>();
    auto saveState = state;
    state = mergeState(io->state);
    if (state == ValueState::Constant && constant->value != io->constant->value) {
        state = ValueState::NotConstant;
        constant = nullptr;
    }
    return (state != saveState);
}

void SymbolicInteger::assign(const SymbolicValue *other) {
    if (other->is<SymbolicError>()) return;
    BUG_CHECK(other->is<SymbolicInteger>(), "%1%: expected an integer", other);
    auto io = other->to<SymbolicInteger>();
    state = io->state;
    constant = io->constant;
}

bool SymbolicInteger::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicInteger>()) return false;
    auto ab = other->to<SymbolicInteger>();
    if (state != ab->state) return false;
    if (isKnown()) return constant->value == ab->constant->value;
    return true;
}

bool SymbolicVarbit::merge(const SymbolicValue *other) {
    BUG_CHECK(other->is<SymbolicVarbit>(), "%1%: expected a varbit", other);
    auto vo = other->to<SymbolicVarbit>();
    auto saveState = state;
    state = mergeState(vo->state);
    return (state != saveState);
}

void SymbolicVarbit::assign(const SymbolicValue *other) {
    if (other->is<SymbolicError>()) return;
    BUG_CHECK(other->is<SymbolicVarbit>(), "%1%: expected a varbit", other);
    auto vo = other->to<SymbolicVarbit>();
    state = vo->state;
}

bool SymbolicVarbit::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicVarbit>()) return false;
    auto ab = other->to<SymbolicVarbit>();
    return state == ab->state;
}

void SymbolicEnum::assign(const SymbolicValue *other) {
    BUG_CHECK(other->is<SymbolicEnum>(), "%1%: expected an enum", other);
    auto bo = other->to<SymbolicEnum>();
    state = bo->state;
    value = bo->value;
}

bool SymbolicEnum::merge(const SymbolicValue *other) {
    auto bo = other->to<SymbolicEnum>();
    auto saveState = state;
    state = mergeState(other->to<ScalarValue>()->state);
    if (state == ValueState::Constant && value != bo->value) state = ValueState::NotConstant;
    return (state != saveState);
}

bool SymbolicEnum::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicEnum>()) return false;
    auto ab = other->to<SymbolicEnum>();
    if (state != ab->state) return false;
    if (isKnown()) return value == ab->value;
    return true;
}

//////////////////////////////////////////////////////////////////////////////////

SymbolicStruct::SymbolicStruct(const IR::Type_StructLike *type, bool uninitialized,
                               const SymbolicValueFactory *factory)
    : SymbolicValue(type) {
    CHECK_NULL(type);
    CHECK_NULL(factory);
    for (auto f : type->fields) {
        auto value = factory->create(f->type, uninitialized);
        fieldValue[f->name.name] = value;
    }
}

SymbolicValue *SymbolicStruct::clone() const {
    auto result = new SymbolicStruct(type->to<IR::Type_StructLike>());
    for (auto f : fieldValue) result->fieldValue[f.first] = f.second->clone();
    return result;
}

void SymbolicStruct::assign(const SymbolicValue *other) {
    if (other->is<SymbolicError>()) return;
    BUG_CHECK(other->is<SymbolicStruct>(), "%1%: expected a struct", other);
    auto sv = other->to<SymbolicStruct>();
    for (auto f : sv->fieldValue) fieldValue[f.first]->assign(f.second);
}

bool SymbolicStruct::merge(const SymbolicValue *other) {
    BUG_CHECK(other->is<SymbolicStruct>(), "%1%: expected a struct", other);
    auto sv = other->to<SymbolicStruct>();
    bool changes = false;
    for (auto f : sv->fieldValue) changes = changes || fieldValue[f.first]->merge(f.second);
    return changes;
}

void SymbolicStruct::setAllUnknown() {
    for (auto f : type->to<IR::Type_StructLike>()->fields)
        fieldValue[f->name.name]->setAllUnknown();
}

bool SymbolicStruct::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicStruct>()) return false;
    auto sv = other->to<SymbolicStruct>();
    for (auto f : sv->fieldValue)
        if (!get(nullptr, f.first)->equals(f.second)) return false;
    return true;
}

bool SymbolicStruct::hasUninitializedParts() const {
    for (auto f : fieldValue)
        if (f.second->hasUninitializedParts()) return true;
    return false;
}

void SymbolicStruct::dbprint(std::ostream &out) const {
    bool first = true;
    out << "{ ";
    for (auto f : fieldValue) {
        if (f.second->is<SymbolicHeader>() || f.second->is<SymbolicStruct>() ||
            f.second->is<SymbolicArray>()) {
            if (!first) out << ", ";
            out << f.first << "=>" << f.second;
            first = false;
        }
    }
    out << " }";
}

SymbolicHeaderUnion::SymbolicHeaderUnion(const IR::Type_HeaderUnion *type, bool uninitialized,
                                         const SymbolicValueFactory *factory)
    : SymbolicStruct(type, uninitialized, factory) {}

SymbolicBool *SymbolicHeaderUnion::isValid() const {
    int validFields = 0;
    for (auto f : type->to<IR::Type_StructLike>()->fields) {
        if (fieldValue.count(f->name.name)) {
            auto fieldValid = fieldValue.at(f->name.name)->checkedTo<SymbolicHeader>()->valid;
            if (!fieldValid->isKnown() || fieldValid->isUninitialized()) {
                return fieldValid;
            } else if (fieldValid->value) {
                validFields += 1;
            }
        } else {
            BUG("The number of fields in %1% is different from HeaderUnion fieldValue", type);
        }
    }
    if (validFields == 1) {
        return new SymbolicBool(true);
    } else if (validFields > 1) {
        BUG("In HeaderUnion cannot be more than one valid field");
    }
    return new SymbolicBool(false);
}

SymbolicValue *SymbolicHeaderUnion::get(const IR::Node *node, cstring field) const {
    return SymbolicStruct::get(node, field);
}

void SymbolicHeaderUnion::setAllUnknown() {
    SymbolicStruct::setAllUnknown();
    this->isValid()->setAllUnknown();
}

SymbolicValue *SymbolicHeaderUnion::clone() const {
    auto result = new SymbolicHeaderUnion(type->to<IR::Type_HeaderUnion>());
    for (auto f : fieldValue) result->fieldValue[f.first] = f.second->clone();
    return result;
}

void SymbolicHeaderUnion::assign(const SymbolicValue *other) {
    if (other->is<SymbolicError>()) return;
    auto hv = other->to<SymbolicHeaderUnion>();
    BUG_CHECK(hv, "%1%: expected a header union", other);
    for (auto f : hv->fieldValue) fieldValue[f.first]->assign(f.second);
}

bool SymbolicHeaderUnion::merge(const SymbolicValue *other) {
    auto hv = other->to<SymbolicHeaderUnion>();
    BUG_CHECK(hv, "%1%: expected a header union", other);
    bool changes = false;
    for (auto f : hv->fieldValue) changes = changes || fieldValue[f.first]->merge(f.second);
    return changes;
}

bool SymbolicHeaderUnion::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicHeaderUnion>()) return false;
    return SymbolicStruct::equals(other);
}

void SymbolicHeaderUnion::dbprint(std::ostream &out) const {
    out << "{ ";
#if 0
    for (auto f : fieldValue) {
        out << ", ";
        out << f.first << "=>" << f.second;
    }
#endif
    out << " }";
}

SymbolicHeader::SymbolicHeader(const IR::Type_Header *type, bool uninitialized,
                               const SymbolicValueFactory *factory)
    : SymbolicStruct(type, uninitialized, factory), valid(new SymbolicBool(false)) {}

void SymbolicHeader::setValid(bool v) {
    if (!v) setAllUnknown();
    valid = new SymbolicBool(v);
}

SymbolicValue *SymbolicHeader::get(const IR::Node *node, cstring field) const {
    if (valid->isKnown() && !valid->value)
        return new SymbolicStaticError(node, "Reading field from invalid header");
    return SymbolicStruct::get(node, field);
}

void SymbolicHeader::setAllUnknown() {
    SymbolicStruct::setAllUnknown();
    valid->setAllUnknown();
}

SymbolicValue *SymbolicHeader::clone() const {
    auto result = new SymbolicHeader(type->to<IR::Type_Header>());
    for (auto f : fieldValue) result->fieldValue[f.first] = f.second->clone();
    result->valid = valid->clone()->to<SymbolicBool>();
    return result;
}

void SymbolicHeader::assign(const SymbolicValue *other) {
    if (other->is<SymbolicError>()) return;
    BUG_CHECK(other->is<SymbolicStruct>(), "%1%: expected a struct", other);
    if (auto hv = other->to<SymbolicStruct>()) {
        for (auto f : hv->fieldValue) fieldValue[f.first]->assign(f.second);
    }
    if (auto hv = other->to<SymbolicHeader>())
        valid->assign(hv->valid);
    else
        valid->assign(new SymbolicBool(true));
}

bool SymbolicHeader::merge(const SymbolicValue *other) {
    BUG_CHECK(other->is<SymbolicHeader>(), "%1%: expected a header", other);
    auto hv = other->to<SymbolicHeader>();
    bool changes = false;
    for (auto f : hv->fieldValue) changes = changes || fieldValue[f.first]->merge(f.second);
    changes = changes || valid->merge(hv->valid);
    return changes;
}

bool SymbolicHeader::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicHeader>()) return false;
    auto sh = other->to<SymbolicHeader>();
    if (!valid->equals(sh->valid)) return false;
    if (valid->isKnown() && !valid->value)
        // Invalid headers are equal
        return true;
    return SymbolicStruct::equals(other);
}

void SymbolicHeader::dbprint(std::ostream &out) const {
    out << "{ ";
    out << "valid=>";
    valid->dbprint(out);
#if 0
    for (auto f : fieldValue) {
        out << ", ";
        out << f.first << "=>" << f.second;
    }
#endif
    out << " }";
}

SymbolicArray::SymbolicArray(const IR::Type_Stack *type, bool uninitialized,
                             const SymbolicValueFactory *factory)
    : SymbolicValue(type),
      size(type->getSize()),
      elemType(type->elementType->to<IR::Type_Header>()) {
    for (unsigned i = 0; i < size; i++) {
        if (type->elementType->to<IR::Type_Header>()) {
            auto elem = factory->create(elemType, uninitialized);
            BUG_CHECK(elem->is<SymbolicHeader>(), "%1%: expected a header", elem);
            values.push_back(elem->to<SymbolicHeader>());
        }
        if (auto newElemType = type->elementType->to<IR::Type_HeaderUnion>()) {
            auto elem = factory->create(newElemType, uninitialized);
            BUG_CHECK(elem->is<SymbolicHeaderUnion>(), "%1%: expected a header union", elem);
            values.push_back(elem->to<SymbolicHeaderUnion>());
        }
    }
}

void SymbolicArray::shift(int amount) {
    if (amount < 0) {
        for (unsigned i = 0; i < values.size() + amount; i++) values[i] = values[i - amount];
        for (unsigned i = values.size() + amount; i < values.size(); i++) {
            if (values[i]->is<SymbolicHeader>()) {
                values[i]->to<SymbolicHeader>()->setValid(false);
            }
        }
    } else if (amount > 0) {
        for (unsigned i = 0; i < values.size() - amount; i++)
            values[values.size() - i - 1] = values[values.size() - i - amount - 1];
        for (unsigned i = 0; i < (unsigned)amount; i++) {
            if (values[i]->is<SymbolicHeader>()) {
                values[i]->to<SymbolicHeader>()->setValid(false);
            }
        }
    }
}

SymbolicValue *SymbolicArray::next(const IR::Node *node) {
    for (unsigned i = 0; i < values.size(); i++) {
        auto v = values.at(i);
        if (values[i]->is<SymbolicHeader>()) {
            if (v->to<SymbolicHeader>()->valid->isUnknown() ||
                v->to<SymbolicHeader>()->valid->isUninitialized())
                return new AnyElement(this);
            if (!v->to<SymbolicHeader>()->valid->value) return v;
        }
        if (values[i]->is<SymbolicHeaderUnion>()) {
            return v;
        }
    }
    return new SymbolicException(node, P4::StandardExceptions::StackOutOfBounds);
}

SymbolicValue *SymbolicArray::lastIndex(const IR::Node *node) {
    for (unsigned i = 0; i < values.size(); i++) {
        unsigned index = values.size() - i - 1;
        auto v = values.at(index);
        if (values[i]->is<SymbolicHeader>()) {
            if (v->to<SymbolicHeader>()->valid->isUnknown() ||
                v->to<SymbolicHeader>()->valid->isUninitialized())
                return new AnyElement(this);
            if (v->to<SymbolicHeader>()->valid->value)
                return new SymbolicInteger(new IR::Constant(IR::Type_Bits::get(32), index));
        }

        if (values[i]->is<SymbolicHeaderUnion>()) {
            return new SymbolicInteger(new IR::Constant(IR::Type_Bits::get(32), index));
        }
    }
    return new SymbolicException(node, P4::StandardExceptions::StackOutOfBounds);
}

SymbolicValue *SymbolicArray::last(const IR::Node *node) {
    for (unsigned i = 0; i < values.size(); i++) {
        unsigned index = values.size() - i - 1;
        auto v = values.at(index);
        if (values[i]->is<SymbolicHeader>()) {
            if (v->to<SymbolicHeader>()->valid->isUnknown() ||
                v->to<SymbolicHeader>()->valid->isUninitialized())
                return new AnyElement(this);
            if (v->to<SymbolicHeader>()->valid->value) return v;
        }
        if (values[i]->is<SymbolicHeaderUnion>()) {
            return v;
        }
    }
    return new SymbolicException(node, P4::StandardExceptions::StackOutOfBounds);
}

void SymbolicArray::setAllUnknown() {
    for (unsigned i = 0; i < values.size(); i++) values.at(i)->setAllUnknown();
}

SymbolicValue *SymbolicArray::clone() const {
    auto result = new SymbolicArray(type->to<IR::Type_Stack>());
    for (unsigned i = 0; i < values.size(); i++)
        result->values.push_back(get(nullptr, i)->clone()->to<SymbolicStruct>());
    return result;
}

void SymbolicArray::assign(const SymbolicValue *other) {
    if (other->is<SymbolicError>()) return;
    BUG_CHECK(other->is<SymbolicArray>(), "%1%: expected an array", other);
    for (unsigned i = 0; i < values.size(); i++)
        values.at(i)->assign(other->to<SymbolicArray>()->get(nullptr, i));
}

bool SymbolicArray::merge(const SymbolicValue *other) {
    BUG_CHECK(other->is<SymbolicArray>(), "%1%: expected an array", other);
    bool changes = false;
    for (unsigned i = 0; i < values.size(); i++)
        changes = changes || values.at(i)->merge(other->to<SymbolicArray>()->get(nullptr, i));
    return changes;
}

bool SymbolicArray::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicArray>()) return false;
    auto sa = other->to<SymbolicArray>();
    for (unsigned i = 0; i < values.size(); i++) {
        if (!values.at(i)->equals(sa->get(nullptr, i))) return false;
    }
    return true;
}

bool SymbolicArray::hasUninitializedParts() const {
    for (unsigned i = 0; i < values.size(); i++)
        if (values.at(i)->hasUninitializedParts()) return true;
    return false;
}

void SymbolicArray::dbprint(std::ostream &out) const {
    bool first = true;
    unsigned index = 0;
    out << "[";
    for (auto f : values) {
        if (!first) out << ", ";
        out << "@" << index << "=";
        out << f;
        first = false;
        index++;
    }
    out << "]";
}

bool AnyElement::merge(const SymbolicValue *) {
    BUG("Merge should not be called on AnyElement");
    return false;
}

bool AnyElement::equals(const SymbolicValue *) const {
    BUG("Equals should not be called on AnyElement");
    return true;
}

SymbolicValue *AnyElement::collapse() const {
    auto result = parent->get(nullptr, 0)->clone();
    for (size_t i = 1; i < parent->values.size(); i++) (void)result->merge(parent->get(nullptr, i));
    return result;
}

SymbolicTuple::SymbolicTuple(const IR::Type_Tuple *type, bool uninitialized,
                             const SymbolicValueFactory *factory)
    : SymbolicValue(type) {
    for (auto t : type->components) {
        auto v = factory->create(t, uninitialized);
        values.push_back(v);
    }
}

void SymbolicTuple::setAllUnknown() {
    for (unsigned i = 0; i < values.size(); i++) values.at(i)->setAllUnknown();
}

SymbolicValue *SymbolicTuple::clone() const {
    auto result = new SymbolicTuple(type->to<IR::Type_Tuple>());
    for (unsigned i = 0; i < values.size(); i++) result->values.push_back(get(i)->clone());
    return result;
}

bool SymbolicTuple::merge(const SymbolicValue *other) {
    BUG_CHECK(other->is<SymbolicTuple>(), "%1%: expected a tuple value", other);
    auto tpl = other->to<SymbolicTuple>();
    BUG_CHECK(values.size() == tpl->values.size(), "merging tuples with different sizes");
    bool changes = false;
    for (unsigned i = 0; i < values.size(); i++)
        changes = changes || values.at(i)->merge(tpl->get(i));
    return changes;
}

bool SymbolicTuple::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicTuple>()) return false;
    auto st = other->to<SymbolicTuple>();
    for (unsigned i = 0; i < values.size(); i++)
        if (!values.at(i)->equals(st->values.at(i))) return false;
    return true;
}

bool SymbolicTuple::hasUninitializedParts() const {
    for (unsigned i = 0; i < values.size(); i++)
        if (values.at(i)->hasUninitializedParts()) return true;
    return false;
}

bool SymbolicExtern::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicExtern>()) return false;
    return true;
}

bool SymbolicPacketIn::merge(const SymbolicValue *other) {
    BUG_CHECK(other->is<SymbolicPacketIn>(), "%1%: merging with non-packet", other);
    auto pv = other->to<SymbolicPacketIn>();
    bool changes = false;
    if (minimumStreamOffset > pv->minimumStreamOffset) {
        minimumStreamOffset = pv->minimumStreamOffset;
        changes = true;
        if (pv->conservative) conservative = true;
    }
    return changes;
}

bool SymbolicPacketIn::equals(const SymbolicValue *other) const {
    if (!other->is<SymbolicPacketIn>()) return false;
    auto sp = other->to<SymbolicPacketIn>();
    // ignore the conservative flag
    return minimumStreamOffset == sp->minimumStreamOffset;
}

SymbolicVoid *SymbolicVoid::instance = new SymbolicVoid();

/*****************************************************************************************/

const IR::Expression *getConstant(const ScalarValue *constant) {
    if (constant->is<SymbolicBool>()) {
        return new IR::BoolLiteral(IR::Type::Boolean::get(), constant->to<SymbolicBool>()->value);
    } else if (constant->is<SymbolicInteger>()) {
        return constant->to<SymbolicInteger>()->constant;
    }
    BUG("Unimplemented structure for expression evaluation %1%", constant);
}

void ExpressionEvaluator::checkResult(const IR::Expression *expression,
                                      const IR::Expression *result) {
    if (result->is<IR::Constant>()) {
        set(expression, new SymbolicInteger(result->to<IR::Constant>()));
        return;
    } else if (result->is<IR::BoolLiteral>()) {
        set(expression, new SymbolicBool(result->to<IR::BoolLiteral>()->value));
        return;
    }
    BUG("%1% : expected a constant/bool literal", result);
}

void ExpressionEvaluator::setNonConstant(const IR::Expression *expression) {
    auto type = typeMap->getType(expression, true);
    if (type->is<IR::Type_Boolean>()) {
        set(expression, new SymbolicBool(ScalarValue::ValueState::NotConstant));
    } else if (type->is<IR::Type_Bits>()) {
        set(expression,
            new SymbolicInteger(ScalarValue::ValueState::NotConstant, type->to<IR::Type_Bits>()));
    } else {
        BUG("Non Type_Bits type %1% for expression %2%", type, expression);
    }
}

void ExpressionEvaluator::postorder(const IR::Operation_Ternary *expression) {
    auto e0 = get(expression->e0);
    if (e0->is<SymbolicError>()) {
        set(expression, e0);
        return;
    }
    auto e1 = get(expression->e1);
    if (e1->is<SymbolicError>()) {
        set(expression, e1);
        return;
    }
    auto e2 = get(expression->e2);
    if (e2->is<SymbolicError>()) {
        set(expression, e2);
        return;
    }
    auto clone = expression->clone();
    BUG_CHECK(e0->is<ScalarValue>(), "%1%: expected an ScalarValue", e0);
    BUG_CHECK(e1->is<ScalarValue>(), "%1%: expected an ScalarValue", e1);
    BUG_CHECK(e2->is<ScalarValue>(), "%1%: expected an ScalarValue", e2);
    auto e0i = e0->to<ScalarValue>();
    auto e1i = e1->to<ScalarValue>();
    auto e2i = e2->to<ScalarValue>();
    if (e0i->isUninitialized()) {
        auto result = new SymbolicStaticError(expression->e0, "Uninitialized");
        set(expression, result);
        return;
    } else if (e1i->isUninitialized()) {
        auto result = new SymbolicStaticError(expression->e1, "Uninitialized");
        set(expression, result);
        return;
    } else if (e2i->isUninitialized()) {
        auto result = new SymbolicStaticError(expression->e2, "Uninitialized");
        set(expression, result);
        return;
    } else if (!e0i->isUnknown() && !e1i->isUnknown() && !e2i->isUnknown()) {
        clone->e0 = getConstant(e0i);
        clone->e1 = getConstant(e1i);
        clone->e2 = getConstant(e2i);
        DoConstantFolding cf(refMap, typeMap);
        cf.setCalledBy(this);
        auto result = clone->apply(cf);
        checkResult(expression, result);
        return;
    }
    setNonConstant(expression);
}

void ExpressionEvaluator::postorder(const IR::Operation_Binary *expression) {
    auto l = get(expression->left);
    if (l->is<SymbolicError>()) {
        set(expression, l);
        return;
    }
    auto r = get(expression->right);
    if (r->is<SymbolicError>()) {
        set(expression, l);
        return;
    }
    auto clone = expression->clone();
    BUG_CHECK(l->is<ScalarValue>(), "%1%: expected an ScalarValue", l);
    BUG_CHECK(r->is<ScalarValue>(), "%1%: expected an ScalarValue", r);
    auto li = l->to<ScalarValue>();
    auto ri = r->to<ScalarValue>();
    if (li->isUninitialized()) {
        auto result = new SymbolicStaticError(expression->left, "Uninitialized");
        set(expression, result);
        return;
    } else if (ri->isUninitialized()) {
        auto result = new SymbolicStaticError(expression->right, "Uninitialized");
        set(expression, result);
        return;
    } else if (!li->isUnknown() && !ri->isUnknown()) {
        clone->left = getConstant(li);
        clone->right = getConstant(ri);
        DoConstantFolding cf(refMap, typeMap);
        cf.setCalledBy(this);
        auto result = clone->apply(cf);
        checkResult(expression, result);
        return;
    }
    setNonConstant(expression);
}

void ExpressionEvaluator::postorder(const IR::Operation_Unary *expression) {
    auto l = get(expression->expr);
    if (l->is<SymbolicError>()) {
        set(expression, l);
        return;
    }
    auto clone = expression->clone();
    BUG_CHECK(l->is<ScalarValue>(), "%1%: expected a scalar", l);
    auto sv = l->to<ScalarValue>();
    if (sv->isUninitialized()) {
        auto result = new SymbolicStaticError(expression->expr, "Uninitialized");
        set(expression, result);
        return;
    }
    if (sv->isUnknown()) {
        if (auto cast = expression->to<IR::Cast>()) {
            if (cast->destType->is<IR::Type_Boolean>() && l->is<SymbolicInteger>()) {
                l = new SymbolicBool(sv->state);
            } else if (cast->destType->is<IR::Type_Bits>() && l->is<SymbolicBool>()) {
                l = new SymbolicInteger(sv->state, cast->destType->to<IR::Type_Bits>());
            } else {
                BUG_CHECK(!l->is<SymbolicInteger>() || !l->is<SymbolicBool>(),
                          "%1% unexpected type %2% in cast", cast, cast->destType);
            }
        }
        set(expression, l);
        return;
    }

    if (l->is<SymbolicInteger>()) {
        auto li = l->to<SymbolicInteger>();
        clone->expr = li->constant;
    } else if (l->is<SymbolicBool>()) {
        auto li = l->to<SymbolicBool>();
        clone->expr = new IR::BoolLiteral(li->value);
    } else {
        BUG("%1%: unexpected type", l);
    }

    auto type = typeMap->getType(getOriginal(), true);
    typeMap->setType(clone, type);  // needed by the constant folding
    DoConstantFolding cf(refMap, typeMap);
    cf.setCalledBy(this);
    auto result = clone->apply(cf);

    if (type->is<IR::Type_Bits>()) {
        BUG_CHECK(result->is<IR::Constant>(), "%1%: expected a constant", result);
        set(expression, new SymbolicInteger(result->to<IR::Constant>()));
    } else if (type->is<IR::Type_Boolean>()) {
        BUG_CHECK(result->is<IR::BoolLiteral>(), "%1%: expected a boolean", result);
        set(expression, new SymbolicBool(result->to<IR::BoolLiteral>()));
    } else {
        BUG("%1%: unexpected type", type);
    }
}

void ExpressionEvaluator::postorder(const IR::Constant *expression) {
    set(expression, new SymbolicInteger(expression));
}

void ExpressionEvaluator::postorder(const IR::ListExpression *expression) {
    auto type = typeMap->getType(expression, true);
    auto result = new SymbolicTuple(type->to<IR::Type_Tuple>());
    for (auto e : expression->components) {
        auto v = get(e);
        result->add(v);
    }
    set(expression, result);
}

void ExpressionEvaluator::postorder(const IR::StructExpression *expression) {
    auto type = typeMap->getType(expression, true);
    auto result = new SymbolicStruct(type->to<IR::Type_StructLike>());
    for (auto e : expression->components) {
        auto v = get(e->expression);
        result->set(e->name, v);
    }
    set(expression, result);
}

void ExpressionEvaluator::postorder(const IR::BoolLiteral *expression) {
    set(expression, new SymbolicBool(expression));
}

void ExpressionEvaluator::postorder(const IR::Operation_Relation *expression) {
    auto l = get(expression->left);
    if (l->is<SymbolicError>()) {
        set(expression, l);
        return;
    }
    auto r = get(expression->right);
    if (r->is<SymbolicError>()) {
        set(expression, l);
        return;
    }
    BUG_CHECK(l->is<ScalarValue>(), "%1%: expected a scalar", l);
    BUG_CHECK(r->is<ScalarValue>(), "%1%: expected a scalar", r);
    auto lv = l->to<ScalarValue>();
    auto rv = r->to<ScalarValue>();
    if (lv->isUninitialized()) {
        auto result = new SymbolicStaticError(expression->left, "Uninitialized");
        set(expression, result);
        return;
    }
    if (rv->isUninitialized()) {
        auto result = new SymbolicStaticError(expression->right, "Uninitialized");
        set(expression, result);
        return;
    }
    if (lv->isUnknown()) {
        set(expression, new SymbolicBool(ScalarValue::ValueState::NotConstant));
        return;
    }
    if (rv->isUnknown()) {
        set(expression, new SymbolicBool(ScalarValue::ValueState::NotConstant));
        return;
    }

    auto clone = expression->clone();
    if (l->is<SymbolicInteger>()) {
        BUG_CHECK(r->is<SymbolicInteger>(), "%1%: expected an SymbolicInteger");
        clone->left = l->to<SymbolicInteger>()->constant;
        clone->right = r->to<SymbolicInteger>()->constant;
        DoConstantFolding cf(refMap, typeMap);
        cf.setCalledBy(this);
        auto result = clone->apply(cf);
        BUG_CHECK(result->is<IR::BoolLiteral>(), "%1%: expected a boolean", result);
        set(expression, new SymbolicBool(result->to<IR::BoolLiteral>()));
        return;
    } else if (l->is<SymbolicBool>()) {
        BUG_CHECK(r->is<SymbolicBool>(), "%1%: expected an SymbolicBool");
        clone->left = new IR::BoolLiteral(l->to<SymbolicBool>()->value);
        clone->right = new IR::BoolLiteral(r->to<SymbolicBool>()->value);
        DoConstantFolding cf(refMap, typeMap);
        cf.setCalledBy(this);
        auto result = clone->apply(cf);
        BUG_CHECK(result->is<IR::BoolLiteral>(), "%1%: expected a boolean", result);
        set(expression, new SymbolicBool(result->to<IR::BoolLiteral>()));
        return;
    }
    BUG("%1%: unexpected type", l);
}

void ExpressionEvaluator::postorder(const IR::Member *expression) {
    auto type = typeMap->getType(expression, true);
    if (type->is<IR::Type_Error>()) {
        set(expression, new SymbolicEnum(expression->type, expression->member));
        return;
    }
    if (type->is<IR::Type_MethodBase>()) {
        // not really void, but we can't do anything with this anyway
        set(expression, SymbolicVoid::get());
        return;
    }
    auto l = get(expression->expr);
    if (l->is<SymbolicError>()) {
        set(expression, l);
        return;
    }
    auto basetype = typeMap->getType(expression->expr, true);
    if (basetype->is<IR::Type_Stack>()) {
        BUG_CHECK(l->is<SymbolicArray>(), "%1%: expected an array", l);
        auto array = l->to<SymbolicArray>();
        SymbolicValue *v;
        if (expression->member.name == IR::Type_Stack::next) {
            v = array->next(expression);
            if (v->is<SymbolicError>()) {
                set(expression, v);
                return;
            }
        } else if (expression->member.name == IR::Type_Stack::last) {
            v = array->last(expression);
            if (v->is<SymbolicError>()) {
                set(expression, v);
                return;
            }
        } else if (expression->member.name == IR::Type_Stack::lastIndex) {
            v = array->lastIndex(expression);
            if (v->is<SymbolicError>()) {
                set(expression, v);
                return;
            }
        } else {
            BUG("%1%: unexpected expression", expression);
        }
        set(expression, v);
    } else if (basetype->is<IR::Type_HeaderUnion>()) {
        BUG_CHECK(l->is<SymbolicHeaderUnion>(), "%1%: expected a header union", l);
        auto v = l->to<SymbolicHeaderUnion>()->get(expression, expression->member.name);
        set(expression, v);
    } else {
        BUG_CHECK(l->is<SymbolicStruct>(), "%1%: expected a struct", l);
        auto v = l->to<SymbolicStruct>()->get(expression, expression->member.name);
        set(expression, v);
    }
}

bool ExpressionEvaluator::preorder(const IR::ArrayIndex *expression) {
    bool lv = evaluatingLeftValue;
    evaluatingLeftValue = false;
    visit(expression->left);
    evaluatingLeftValue = lv;
    visit(expression->right);
    return true;  // don't prune
}

void ExpressionEvaluator::postorder(const IR::ArrayIndex *expression) {
    auto l = get(expression->left);
    auto r = get(expression->right);

    if (l->is<SymbolicError>()) {
        set(expression, l);
        return;
    }
    if (r->is<SymbolicError>()) {
        set(expression, r);
        return;
    }
    auto rv = r->to<ScalarValue>();
    auto lv = l->to<SymbolicArray>();

    if (rv->isUninitialized() || rv->isUnknown()) {
        if (rv->isUninitialized()) {
            auto result = new SymbolicStaticError(expression->right, "Uninitialized");
            set(expression, result);
            return;
        }
        auto v0 = new AnyElement(lv);
        if (!evaluatingLeftValue)
            set(expression, v0->collapse());
        else
            set(expression, v0);
        return;
    }
    CHECK_NULL(lv);
    auto ix = r->to<SymbolicInteger>();
    CHECK_NULL(ix);
    auto result = lv->get(expression, ix->constant->asInt());
    set(expression, result);
}

void ExpressionEvaluator::postorder(const IR::PathExpression *expression) {
    auto type = typeMap->getType(expression, true);
    auto decl = refMap->getDeclaration(expression->path, true);
    SymbolicValue *result;
    if (type->is<IR::Type_Error>())
        result = new SymbolicEnum(type, decl->getName());
    else
        result = valueMap->get(decl);
    set(expression, result);
}

void ExpressionEvaluator::postorder(const IR::MethodCallExpression *expression) {
    MethodInstance *mi = MethodInstance::resolve(expression, refMap, typeMap);
    for (auto arg : *expression->arguments) {
        auto argValue = get(arg->expression);
        CHECK_NULL(argValue);
        if (argValue->is<SymbolicError>()) {
            set(expression, argValue);
            return;
        }
        // TODO: check for uninitialized in arguments
    }

    if (mi->is<BuiltInMethod>()) {
        auto bim = mi->to<BuiltInMethod>();
        auto base = get(bim->appliedTo);
        cstring name = bim->name.name;
        // Needed to get Header from HeaderUnion
        const auto node = expression->method->checkedTo<IR::Member>()->expr;
        CHECK_NULL(node);
        auto structVar = get(node);
        if (name == IR::Type_Header::setInvalid || name == IR::Type_Header::setValid) {
            auto hv = structVar->checkedTo<SymbolicHeader>();
            if (auto member = node->to<IR::Member>()) {
                if (auto hu = get(member->expr)->to<SymbolicHeaderUnion>()) {
                    if (hu->isValid()) {
                        hu->setAllUnknown();
                    }
                }
            }
            hv->setValid(name == IR::Type_Header::setValid);
            set(expression, SymbolicVoid::get());
            return;
        } else if (name == IR::Type_Stack::push_front || name == IR::Type_Stack::pop_front) {
            BUG_CHECK(base->is<SymbolicArray>(), "%1%: expected an array", base);
            auto array = base->to<SymbolicArray>();
            BUG_CHECK(expression->arguments->size() == 1, "%1%: not one argument?", expression);
            auto amount = get(expression->arguments->at(0)->expression);
            BUG_CHECK(amount->is<SymbolicInteger>(), "%1%: expected an integer", amount);
            auto ac = amount->to<SymbolicInteger>();
            if (ac->isUnknown()) {
                array->setAllUnknown();
                return;
            }
            BUG_CHECK(amount->is<SymbolicInteger>(), "%1%: expected an integer", amount);
            int amt = amount->to<SymbolicInteger>()->constant->asInt();
            if (name == IR::Type_Stack::pop_front) amt = -amt;
            array->shift(amt);
            set(expression, SymbolicVoid::get());
            return;
        } else {
            BUG_CHECK(name == IR::Type_Header::isValid, "%1%: unexpected method", bim->name);
            if (auto hv = structVar->to<SymbolicHeader>()) {
                auto v = hv->valid;
                set(expression, v);
                return;
            } else {
                BUG("Unexpected expression (%1%) type: %2%", base, base->type);
            }
        }
    }

    if (mi->is<ExternMethod>()) {
        // There are some extern methods that we know something about
        auto em = mi->to<ExternMethod>();
        if (em->originalExternType->name.name == P4CoreLibrary::instance().packetIn.name) {
            // packet methods
            if (em->method->name.name == P4CoreLibrary::instance().packetIn.extract.name) {
                // We know that after an extract terminates the header argument
                // is always valid.
                auto arg0 = expression->arguments->at(0);
                auto argType = typeMap->getType(arg0, true);
                auto hdr = get(arg0->expression);
                bool fixed = factory->isFixedWidth(argType);
                unsigned width = factory->getWidth(argType);
                // For variable-sized objects width is the "minimum" width.

                if (expression->arguments->size() == 1) {
                    // 1-argument extract method
                    if (!fixed || !argType->is<IR::Type_Header>()) {
                        auto result = new SymbolicStaticError(
                            arg0, "Expected a fixed-size header as argument");
                        set(expression, result);
                        return;
                    }
                } else {
                    BUG_CHECK(expression->arguments->size() == 2, "%1%: expected 2 arguments",
                              expression);
                    // TODO: check first argument type more in depth; i.e. only
                    // one varbit field allowed
                    if (fixed || !argType->is<IR::Type_Header>()) {
                        auto result = new SymbolicStaticError(
                            arg0, "Expected a variable-size header as argument");
                        set(expression, result);
                        return;
                    }
                    auto arg1 = expression->arguments->at(1);
                    auto sz = get(arg1->expression);
                    if (sz->is<SymbolicInteger>()) {
                        auto szValue = sz->to<SymbolicInteger>();
                        if (szValue->isKnown())
                            width = static_cast<unsigned>(szValue->constant->asInt());
                    }
                }

                auto decl = em->object;
                auto obj = valueMap->get(decl);
                CHECK_NULL(obj);
                if (obj->is<SymbolicError>()) {
                    set(expression, obj);
                    return;
                }

                BUG_CHECK(obj->is<SymbolicPacketIn>(), "%1%: expected a packetIn", decl);
                auto pkt = obj->to<SymbolicPacketIn>();
                pkt->advance(width);
                if (!fixed) pkt->setConservative();

                BUG_CHECK(hdr->is<SymbolicHeader>(), "%1%: Not a header?", hdr);
                auto sh = hdr->to<SymbolicHeader>();
                if (sh->valid->isKnown() && sh->valid->value) {
                    auto result = new SymbolicException(expression,
                                                        P4::StandardExceptions::OverwritingHeader);
                    set(expression, result);
                    return;
                }
                sh->setAllUnknown();
                sh->setValid(true);
                set(expression, SymbolicVoid::get());
                return;
            } else if (em->method->name.name == P4CoreLibrary::instance().packetIn.lookahead.name) {
                // If lookahead returns a header, it is always valid.
                auto type = typeMap->getTypeType(mi->actualMethodType->returnType, true);
                auto res = factory->create(type, false);
                if (auto sh = res->to<SymbolicHeader>()) {
                    sh->setValid(true);
                }
                set(expression, res);
                return;
            }
        }
    }

    // For all other methods we act conservatively:
    // in arguments are unchanged, and the out arguments have an unknown value.
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        if (p->direction == IR::Direction::Out || p->direction == IR::Direction::InOut) {
            auto arg = mi->substitution.lookup(p);
            auto val = get(arg->expression);
            val->setAllUnknown();
        }
    }

    if (mi->actualMethodType->returnType == nullptr ||
        mi->actualMethodType->returnType->is<IR::Type_Void>()) {
        set(expression, SymbolicVoid::get());
    } else {
        auto type = typeMap->getTypeType(mi->actualMethodType->returnType, true);
        auto res = factory->create(type, false);
        set(expression, res);
    }
}

SymbolicValue *ExpressionEvaluator::evaluate(const IR::Expression *expression, bool leftValue) {
    evaluatingLeftValue = leftValue;
    (void)expression->apply(*this);
    auto result = get(expression);
    return result;
}
}  // namespace P4
