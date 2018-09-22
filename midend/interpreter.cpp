#include "interpreter.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/coreLibrary.h"

namespace P4 {

unsigned SymbolicValue::crtid = 0;

SymbolicValue* SymbolicValueFactory::create(const IR::Type* type, bool uninitialized) const {
    type = typeMap->getType(type, true);
    if (type->is<IR::Type_Bits>())
        return new SymbolicInteger(ScalarValue::init(uninitialized), type->to<IR::Type_Bits>());
    if (type->is<IR::Type_Boolean>())
        return new SymbolicBool(ScalarValue::init(uninitialized));
    if (type->is<IR::Type_Struct>())
        return new SymbolicStruct(type->to<IR::Type_Struct>(), uninitialized, this);
    if (type->is<IR::Type_Header>())
        return new SymbolicHeader(type->to<IR::Type_Header>(), uninitialized, this);
    if (type->is<IR::Type_Varbits>())
        return new SymbolicVarbit(type->to<IR::Type_Varbits>());
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
        if (te->name.name == P4CoreLibrary::instance.packetIn.name) {
            return new SymbolicPacketIn(te);
        }
        return new SymbolicExtern(te);
    }
    if (type->is<IR::Type_Enum>() ||
        type->is<IR::Type_Error>() ||
        type->is<IR::Type_MatchKind>()) {
        return new SymbolicEnum(type);
    }
    if (type->is<IR::Type_SpecializedCanonical>()) {
        auto spec = type->to<IR::Type_SpecializedCanonical>();
        return create(spec->substituted, uninitialized);
    }
    if (type->is<IR::Type_Parser>() || type->is<IR::Type_Control>() ||
        type->is<IR::P4Parser>() || type->is<IR::P4Control>())
        // This implies that inlining has not been done;
        // just ignore these values.
        return nullptr;

    BUG("%1%: unexpected type", type);
}

bool SymbolicValueFactory::isFixedWidth(const IR::Type* type) const {
    type = typeMap->getType(type, true);
    if (type->is<IR::Type_Varbits>())
        return false;
    if (type->is<IR::Type_Extern>())
        return false;
    if (type->is<IR::Type_Stack>())
        return isFixedWidth(type->to<IR::Type_Stack>()->elementType);
    if (type->is<IR::Type_StructLike>()) {
        auto st = type->to<IR::Type_StructLike>();
        for (auto f : st->fields)
            if (!isFixedWidth(f->type))
                return false;
        return true;
    }
    if (type->is<IR::Type_Tuple>()) {
        auto tt = type->to<IR::Type_Tuple>();
        for (auto f : tt->components) {
            if (!isFixedWidth(f))
                return false;
        }
        return true;
    }
    return true;
}

unsigned SymbolicValueFactory::getWidth(const IR::Type* type) const {
    type = typeMap->getType(type, true);
    if (type->is<IR::Type_Bits>())
        return type->to<IR::Type_Bits>()->size;
    if (type->is<IR::Type_Boolean>())
        return 1;
    if (type->is<IR::Type_HeaderUnion>()) {
        unsigned width = 0;
        for (auto f : type->to<IR::Type_HeaderUnion>()->fields)
            width = std::max(width, getWidth(f->type));
        return width;
    }
    if (type->is<IR::Type_StructLike>()) {
        unsigned width = 0;
        auto st = type->to<IR::Type_StructLike>();
        for (auto f : st->fields)
            width += getWidth(f->type);
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
        for (auto f : tt->components)
            width += getWidth(f);
        return width;
    }
    return 0;
}

/*********************************************************************************/

bool SymbolicException::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicException>())
        return false;
    return (other->to<SymbolicException>()->exc == exc);
}

bool SymbolicStaticError::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicStaticError>())
        return false;
    return msg == other->to<SymbolicStaticError>()->msg;
}

bool SymbolicBool::merge(const SymbolicValue* other) {
    auto bo = other->to<SymbolicBool>();
    auto saveState = state;
    state = mergeState(other->to<ScalarValue>()->state);
    if (state == ValueState::Constant && value != bo->value)
        state = ValueState::NotConstant;
    return (state != saveState);
}

void SymbolicBool::assign(const SymbolicValue* other) {
    BUG_CHECK(other->is<SymbolicBool>(), "%1%: expected a bool", other);
    auto bo = other->to<SymbolicBool>();
    state = bo->state;
    value = bo->value;
}

bool SymbolicBool::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicBool>())
        return false;
    auto ab = other->to<SymbolicBool>();
    if (state != ab->state)
        return false;
    if (isKnown())
        return value == ab->value;
    return true;
}

bool SymbolicInteger::merge(const SymbolicValue* other) {
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

void SymbolicInteger::assign(const SymbolicValue* other) {
    if (other->is<SymbolicError>()) return;
    BUG_CHECK(other->is<SymbolicInteger>(), "%1%: expected an integer", other);
    auto io = other->to<SymbolicInteger>();
    state = io->state;
    constant = io->constant;
}

bool SymbolicInteger::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicInteger>())
        return false;
    auto ab = other->to<SymbolicInteger>();
    if (state != ab->state)
        return false;
    if (isKnown())
        return constant->value == ab->constant->value;
    return true;
}

bool SymbolicVarbit::merge(const SymbolicValue* other) {
    BUG_CHECK(other->is<SymbolicVarbit>(), "%1%: expected a varbit", other);
    auto vo = other->to<SymbolicVarbit>();
    auto saveState = state;
    state = mergeState(vo->state);
    return (state != saveState);
}

void SymbolicVarbit::assign(const SymbolicValue* other) {
    if (other->is<SymbolicError>()) return;
    BUG_CHECK(other->is<SymbolicVarbit>(), "%1%: expected a varbit", other);
    auto vo = other->to<SymbolicVarbit>();
    state = vo->state;
}

bool SymbolicVarbit::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicVarbit>())
        return false;
    auto ab = other->to<SymbolicVarbit>();
    return state == ab->state;
}

void SymbolicEnum::assign(const SymbolicValue* other) {
    BUG_CHECK(other->is<SymbolicEnum>(), "%1%: expected an enum", other);
    auto bo = other->to<SymbolicEnum>();
    state = bo->state;
    value = bo->value;
}

bool SymbolicEnum::merge(const SymbolicValue* other) {
    auto bo = other->to<SymbolicEnum>();
    auto saveState = state;
    state = mergeState(other->to<ScalarValue>()->state);
    if (state == ValueState::Constant && value != bo->value)
        state = ValueState::NotConstant;
    return (state != saveState);
}

bool SymbolicEnum::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicEnum>())
        return false;
    auto ab = other->to<SymbolicEnum>();
    if (state != ab->state)
        return false;
    if (isKnown())
        return value == ab->value;
    return true;
}

//////////////////////////////////////////////////////////////////////////////////

SymbolicStruct::SymbolicStruct(const IR::Type_StructLike* type, bool uninitialized,
                               const SymbolicValueFactory* factory) : SymbolicValue(type) {
    CHECK_NULL(type); CHECK_NULL(factory);
    for (auto f : type->fields) {
        auto value = factory->create(f->type, uninitialized);
        fieldValue[f->name.name] = value;
    }
}

SymbolicValue* SymbolicStruct::clone() const {
    auto result = new SymbolicStruct(type->to<IR::Type_StructLike>());
    for (auto f : fieldValue)
        result->fieldValue[f.first] = f.second->clone();
    return result;
}

void SymbolicStruct::assign(const SymbolicValue* other) {
    if (other->is<SymbolicError>()) return;
    BUG_CHECK(other->is<SymbolicStruct>(), "%1%: expected a struct", other);
    auto sv = other->to<SymbolicStruct>();
    for (auto f : sv->fieldValue)
        fieldValue[f.first]->assign(f.second);
}

bool SymbolicStruct::merge(const SymbolicValue* other) {
    BUG_CHECK(other->is<SymbolicStruct>(), "%1%: expected a struct", other);
    auto sv = other->to<SymbolicStruct>();
    bool changes = false;
    for (auto f : sv->fieldValue)
        changes = changes || fieldValue[f.first]->merge(f.second);
    return changes;
}

void SymbolicStruct::setAllUnknown() {
    for (auto f : type->to<IR::Type_StructLike>()->fields)
        fieldValue[f->name.name]->setAllUnknown();
}

bool SymbolicStruct::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicStruct>())
        return false;
    auto sv = other->to<SymbolicStruct>();
    for (auto f : sv->fieldValue)
        if (!get(nullptr, f.first)->equals(f.second))
            return false;
    return true;
}

bool SymbolicStruct::hasUninitializedParts() const {
    for (auto f : fieldValue)
        if (f.second->hasUninitializedParts())
            return true;
    return false;
}

void SymbolicStruct::dbprint(std::ostream& out) const {
    bool first = true;
    out << "{ ";
    for (auto f : fieldValue) {
        if (f.second->is<SymbolicHeader>() ||
            f.second->is<SymbolicStruct>() ||
            f.second->is<SymbolicArray>()) {
            if (!first)
                out << ", ";
            out << f.first << "=>" << f.second;
            first = false;
        }
    }
    out << " }";
}

SymbolicHeader::SymbolicHeader(const IR::Type_Header* type,
                               bool uninitialized,
                               const SymbolicValueFactory* factory) :
        SymbolicStruct(type, uninitialized, factory),
        valid(new SymbolicBool(false)) {}

void SymbolicHeader::setValid(bool v) {
    if (!v)
        setAllUnknown();
    valid = new SymbolicBool(v);
}

SymbolicValue* SymbolicHeader::get(const IR::Node* node, cstring field) const {
    if (valid->isKnown() && !valid->value)
        return new SymbolicStaticError(node, "Reading field from invalid header");
    return SymbolicStruct::get(node, field);
}

void SymbolicHeader::setAllUnknown() {
    SymbolicStruct::setAllUnknown();
    valid->setAllUnknown();
}

SymbolicValue* SymbolicHeader::clone() const {
    auto result = new SymbolicHeader(type->to<IR::Type_Header>());
    for (auto f : fieldValue)
        result->fieldValue[f.first] = f.second->clone();
    result->valid = valid->clone()->to<SymbolicBool>();
    return result;
}

void SymbolicHeader::assign(const SymbolicValue* other) {
    if (other->is<SymbolicError>()) return;
    BUG_CHECK(other->is<SymbolicHeader>(), "%1%: expected a header", other);
    auto hv = other->to<SymbolicHeader>();
    for (auto f : hv->fieldValue)
        fieldValue[f.first]->assign(f.second);
    valid->assign(hv->valid);
}

bool SymbolicHeader::merge(const SymbolicValue* other) {
    BUG_CHECK(other->is<SymbolicHeader>(), "%1%: expected a header", other);
    auto hv = other->to<SymbolicHeader>();
    bool changes = false;
    for (auto f : hv->fieldValue)
        changes = changes || fieldValue[f.first]->merge(f.second);
    changes = changes || valid->merge(hv->valid);
    return changes;
}

bool SymbolicHeader::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicHeader>())
        return false;
    auto sh = other->to<SymbolicHeader>();
    if (!valid->equals(sh->valid))
        return false;
    if (valid->isKnown() && !valid->value)
        // Invalid headers are equal
        return true;
    return SymbolicStruct::equals(other);
}

void SymbolicHeader::dbprint(std::ostream& out) const {
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

SymbolicArray::SymbolicArray(const IR::Type_Stack* type, bool uninitialized,
                             const SymbolicValueFactory* factory) :
        SymbolicValue(type), size(type->getSize()),
        elemType(type->elementType->to<IR::Type_Header>()) {
    for (unsigned i=0; i < size; i++) {
        auto elem = factory->create(elemType, uninitialized);
        BUG_CHECK(elem->is<SymbolicHeader>(), "%1%: expected a header", elem);
        values.push_back(elem->to<SymbolicHeader>());
    }
}

void SymbolicArray::shift(int amount) {
    if (amount < 0) {
        for (unsigned i = 0; i < values.size() + amount; i++)
            values[i] = values[i - amount];
        for (unsigned i = values.size() + amount; i < values.size(); i++)
            values[i]->setValid(false);
    } else if (amount > 0) {
        for (unsigned i = 0; i < values.size() - amount; i++)
            values[values.size() - i - 1] = values[values.size() - i - amount - 1];
        for (unsigned i = 0; i < (unsigned)amount; i++)
            values[i]->setValid(false);
    }
}

SymbolicValue* SymbolicArray::next(const IR::Node* node) {
    for (unsigned i = 0; i < values.size(); i++) {
        auto v = values.at(i);
        if (v->valid->isUnknown() || v->valid->isUninitialized())
            return new AnyElement(this);
        if (!v->valid->value)
            return v;
    }
    return new SymbolicException(node, P4::StandardExceptions::StackOutOfBounds);
}

SymbolicValue* SymbolicArray::last(const IR::Node* node) {
    for (unsigned i = 0; i < values.size(); i++) {
        unsigned index = values.size() - i - 1;
        auto v = values.at(index);
        if (v->valid->isUnknown() || v->valid->isUninitialized())
            return new AnyElement(this);
        if (v->valid->value)
            return v;
    }
    return new SymbolicException(node, P4::StandardExceptions::StackOutOfBounds);
}

void SymbolicArray::setAllUnknown() {
    for (unsigned i = 0; i < values.size(); i++)
        values.at(i)->setAllUnknown();
}

SymbolicValue* SymbolicArray::clone() const {
    auto result = new SymbolicArray(type->to<IR::Type_Stack>());
    for (unsigned i=0; i < values.size(); i++)
        result->values.push_back(get(nullptr, i)->clone()->to<SymbolicHeader>());
    return result;
}

void SymbolicArray::assign(const SymbolicValue* other) {
    if (other->is<SymbolicError>()) return;
    BUG_CHECK(other->is<SymbolicArray>(), "%1%: expected an array", other);
    for (unsigned i=0; i < values.size(); i++)
        values.at(i)->assign(other->to<SymbolicArray>()->get(nullptr, i));
}

bool SymbolicArray::merge(const SymbolicValue* other) {
    BUG_CHECK(other->is<SymbolicArray>(), "%1%: expected an array", other);
    bool changes = false;
    for (unsigned i=0; i < values.size(); i++)
        changes = changes || values.at(i)->merge(other->to<SymbolicArray>()->get(nullptr, i));
    return changes;
}

bool SymbolicArray::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicArray>())
        return false;
    auto sa = other->to<SymbolicArray>();
    for (unsigned i=0; i < values.size(); i++) {
        if (!values.at(i)->equals(sa->get(nullptr, i)))
            return false;
    }
    return true;
}

bool SymbolicArray::hasUninitializedParts() const {
    for (unsigned i=0; i < values.size(); i++)
        if (values.at(i)->hasUninitializedParts())
            return true;
    return false;
}

void SymbolicArray::dbprint(std::ostream& out) const {
    bool first = true;
    unsigned index = 0;
    out << "[";
    for (auto f : values) {
        if (!first)
            out << ", ";
        out << "@" << index << "=";
        out << f;
        first = false;
        index++;
    }
    out << "]";
}

bool AnyElement::merge(const SymbolicValue*) {
    BUG("Merge should not be called on AnyElement");
    return false;
}

bool AnyElement::equals(const SymbolicValue*) const {
    BUG("Equals should not be called on AnyElement");
    return true;
}

SymbolicValue* AnyElement::collapse() const {
    auto result = parent->get(nullptr, 0)->clone();
    for (size_t i = 1; i < parent->values.size(); i++)
        (void)result->merge(parent->get(nullptr, i));
    return result;
}

SymbolicTuple::SymbolicTuple(const IR::Type_Tuple* type, bool uninitialized,
                             const SymbolicValueFactory* factory) :
        SymbolicValue(type) {
    for (auto t : type->components) {
        auto v = factory->create(t, uninitialized);
        values.push_back(v);
    }
}

void SymbolicTuple::setAllUnknown() {
    for (unsigned i = 0; i < values.size(); i++)
        values.at(i)->setAllUnknown();
}

SymbolicValue* SymbolicTuple::clone() const {
    auto result = new SymbolicTuple(type->to<IR::Type_Tuple>());
    for (unsigned i=0; i < values.size(); i++)
        result->values.push_back(get(i)->clone());
    return result;
}

bool SymbolicTuple::merge(const SymbolicValue* other) {
    BUG_CHECK(other->is<SymbolicTuple>(), "%1%: expected a tuple value", other);
    auto tpl = other->to<SymbolicTuple>();
    BUG_CHECK(values.size() == tpl->values.size(), "merging tuples with different sizes");
    bool changes = false;
    for (unsigned i=0; i < values.size(); i++)
        changes = changes || values.at(i)->merge(tpl->get(i));
    return changes;
}

bool SymbolicTuple::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicTuple>())
        return false;
    auto st = other->to<SymbolicTuple>();
    for (unsigned i = 0; i < values.size(); i++)
        if (!values.at(i)->equals(st->values.at(i)))
            return false;
    return true;
}

bool SymbolicTuple::hasUninitializedParts() const {
    for (unsigned i=0; i < values.size(); i++)
        if (values.at(i)->hasUninitializedParts())
            return true;
    return false;
}

bool SymbolicExtern::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicExtern>())
        return false;
    return true;
}

bool SymbolicPacketIn::merge(const SymbolicValue* other) {
    BUG_CHECK(other->is<SymbolicPacketIn>(), "%1%: merging with non-packet", other);
    auto pv = other->to<SymbolicPacketIn>();
    bool changes = false;
    if (minimumStreamOffset > pv->minimumStreamOffset) {
        minimumStreamOffset = pv->minimumStreamOffset;
        changes = true;
        if (pv->conservative)
            conservative = true;
    }
    return changes;
}

bool SymbolicPacketIn::equals(const SymbolicValue* other) const {
    if (!other->is<SymbolicPacketIn>())
        return false;
    auto sp = other->to<SymbolicPacketIn>();
    // ignore the conservative flag
    return minimumStreamOffset == sp->minimumStreamOffset;
}

SymbolicVoid* SymbolicVoid::instance = new SymbolicVoid();

/*****************************************************************************************/

void ExpressionEvaluator::postorder(const IR::Operation_Binary* expression) {
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
    BUG_CHECK(l->is<SymbolicInteger>(), "%1%: expected an SymbolicInteger", l);
    BUG_CHECK(r->is<SymbolicInteger>(), "%1%: expected an SymbolicInteger", r);
    auto li = l->to<SymbolicInteger>();
    auto ri = r->to<SymbolicInteger>();
    if (li->isUninitialized()) {
        auto result = new SymbolicStaticError(expression->left, "Uninitialized");
        set(expression, result);
        return;
    } else if (ri->isUninitialized()) {
        auto result = new SymbolicStaticError(expression->right, "Uninitialized");
        set(expression, result);
        return;
    } else if (!li->isUnknown() && !ri->isUnknown()) {
        clone->left = li->constant;
        clone->right = ri->constant;
        DoConstantFolding cf(refMap, typeMap);
        auto result = clone->apply(cf);
        BUG_CHECK(result->is<IR::Constant>(), "%1%: expected a constant", result);
        set(expression, new SymbolicInteger(result->to<IR::Constant>()));
        return;
    }
    auto type = typeMap->getType(expression, true);
    set(expression, new SymbolicInteger(ScalarValue::ValueState::NotConstant,
                                        type->to<IR::Type_Bits>()));
}

void ExpressionEvaluator::postorder(const IR::Operation_Unary* expression) {
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
        set(expression, l);
        return;
    }

    if (l->is<SymbolicInteger>()) {
        auto li = l->to<SymbolicInteger>();
        clone->expr = li->constant;
        DoConstantFolding cf(refMap, typeMap);
        auto result = expression->apply(cf);
        BUG_CHECK(result->is<IR::Constant>(), "%1%: expected a constant", result);
        set(expression, new SymbolicInteger(result->to<IR::Constant>()));
        return;
    } else if (l->is<SymbolicBool>()) {
        auto li = l->to<SymbolicBool>();
        clone->expr = new IR::BoolLiteral(li->value);
        DoConstantFolding cf(refMap, typeMap);
        auto result = expression->apply(cf);
        BUG_CHECK(result->is<IR::BoolLiteral>(), "%1%: expected a boolean", result);
        set(expression, new SymbolicBool(result->to<IR::BoolLiteral>()));
        return;
    }
    BUG("%1%: unexpected type", l);
}

void ExpressionEvaluator::postorder(const IR::Constant* expression) {
    set(expression, new SymbolicInteger(expression));
}

void ExpressionEvaluator::postorder(const IR::ListExpression* expression) {
    auto type = typeMap->getType(expression, true);
    auto result = new SymbolicTuple(type->to<IR::Type_Tuple>());
    for (auto e : expression->components) {
        auto v = get(e);
        result->add(v);
    }
    set(expression, result);
}

void ExpressionEvaluator::postorder(const IR::BoolLiteral* expression) {
    set(expression, new SymbolicBool(expression));
}

void ExpressionEvaluator::postorder(const IR::Operation_Relation* expression) {
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
        set(expression, l);
        return;
    }
    if (rv->isUnknown()) {
        set(expression, r);
        return;
    }

    auto clone = expression->clone();
    if (l->is<SymbolicInteger>()) {
        BUG_CHECK(r->is<SymbolicInteger>(), "%1%: expected an SymbolicInteger");
        clone->left = l->to<SymbolicInteger>()->constant;
        clone->right = r->to<SymbolicInteger>()->constant;
        DoConstantFolding cf(refMap, typeMap);
        auto result = expression->apply(cf);
        BUG_CHECK(result->is<IR::BoolLiteral>(), "%1%: expected a boolean", result);
        set(expression, new SymbolicBool(result->to<IR::BoolLiteral>()));
        return;
    } else if (l->is<SymbolicBool>()) {
        BUG_CHECK(r->is<SymbolicBool>(), "%1%: expected an SymbolicBool");
        clone->left = new IR::BoolLiteral(l->to<SymbolicBool>()->value);
        clone->right = new IR::BoolLiteral(r->to<SymbolicBool>()->value);
        DoConstantFolding cf(refMap, typeMap);
        auto result = expression->apply(cf);
        BUG_CHECK(result->is<IR::BoolLiteral>(), "%1%: expected a boolean", result);
        set(expression, new SymbolicBool(result->to<IR::BoolLiteral>()));
        return;
    }
    BUG("%1%: unexpected type", l);
}

void ExpressionEvaluator::postorder(const IR::Member* expression) {
    auto type = typeMap->getType(expression, true);
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
        SymbolicValue* v;
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
        } else {
            BUG("%1%: unexpected expression", expression);
        }
        set(expression, v);
    } else {
        BUG_CHECK(l->is<SymbolicStruct>(), "%1%: expected a struct", l);
        auto v = l->to<SymbolicStruct>()->get(expression, expression->member.name);
        set(expression, v);
    }
}

bool ExpressionEvaluator::preorder(const IR::ArrayIndex* expression) {
    bool lv = evaluatingLeftValue;
    evaluatingLeftValue = false;
    visit(expression->left);
    evaluatingLeftValue = lv;
    visit(expression->right);
    return false;  // prune
}

void ExpressionEvaluator::postorder(const IR::ArrayIndex* expression) {
    auto l = get(expression->left);
    auto r = get(expression->right);
    auto rv = r->to<ScalarValue>();
    auto lv = l->to<SymbolicArray>();

    if (lv->is<SymbolicError>()) {
        set(expression, lv);
        return;
    }
    if (rv->is<SymbolicError>()) {
        set(expression, rv);
        return;
    }
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

void ExpressionEvaluator::postorder(const IR::PathExpression* expression) {
    auto type = typeMap->getType(expression, true);
    auto decl = refMap->getDeclaration(expression->path, true);
    SymbolicValue* result;
    if (type->is<IR::Type_Error>())
        result = new SymbolicEnum(type, decl->getName());
    else
        result = valueMap->get(decl);
    set(expression, result);
}

void ExpressionEvaluator::postorder(const IR::MethodCallExpression* expression) {
    MethodInstance* mi = MethodInstance::resolve(expression, refMap, typeMap);
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
        if (name == IR::Type_Header::setInvalid ||
            name == IR::Type_Header::setValid) {
            BUG_CHECK(base->is<SymbolicHeader>(), "%1%: expected a header", base);
            auto hv = base->to<SymbolicHeader>();
            hv->setValid(name == IR::Type_Header::setValid);
            set(expression, SymbolicVoid::get());
            return;
        } else if (name == IR::Type_Stack::push_front ||
                   name == IR::Type_Stack::pop_front) {
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
            if (name == IR::Type_Stack::pop_front)
                amt = -amt;
            array->shift(amt);
            set(expression, SymbolicVoid::get());
            return;
        } else {
            BUG_CHECK(name == IR::Type_Header::isValid,
                      "%1%: unexpected method", bim->name);
            BUG_CHECK(base->is<SymbolicHeader>(), "%1%: expected a header", base);
            auto hv = base->to<SymbolicHeader>();
            auto v = hv->valid;
            set(expression, v);
            return;
        }
    }

    if (mi->is<ExternMethod>()) {
        // There are some extern methods that we know something about
        auto em = mi->to<ExternMethod>();
        if (em->originalExternType->name.name == P4CoreLibrary::instance.packetIn.name) {
            // packet methods
            if (em->method->name.name == P4CoreLibrary::instance.packetIn.extract.name) {
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
                    BUG_CHECK(expression->arguments->size() == 2,
                              "%1%: expected 2 arguments", expression);
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
                if (!fixed)
                    pkt->setConservative();

                BUG_CHECK(hdr->is<SymbolicHeader>(), "%1%: Not a header?", hdr);
                auto sh = hdr->to<SymbolicHeader>();
                if (sh->valid->isKnown() && sh->valid->value) {
                    auto result = new SymbolicException(
                        expression, P4::StandardExceptions::OverwritingHeader);
                    set(expression, result);
                    return;
                }
                sh->setAllUnknown();
                sh->setValid(true);
                set(expression, SymbolicVoid::get());
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
        auto type = typeMap->getType(mi->actualMethodType->returnType, true);
        auto res = factory->create(type, false);
        set(expression, res);
    }
}

SymbolicValue* ExpressionEvaluator::evaluate(const IR::Expression* expression, bool leftValue) {
    evaluatingLeftValue = leftValue;
    (void)expression->apply(*this);
    auto result = get(expression);
    return result;
}

}  // namespace P4
