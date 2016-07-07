#include "interpreter.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/coreLibrary.h"

namespace P4 {

unsigned AbstractValue::crtid = 0;

AbstractValue* ValueFactory::create(const IR::Type* type, bool uninitialized) const {
    type = typeMap->getType(type, true);
    if (type->is<IR::Type_Bits>())
        return new AbstractInteger(ScalarValue::init(uninitialized));
    if (type->is<IR::Type_Boolean>())
        return new AbstractBool(ScalarValue::init(uninitialized));
    if (type->is<IR::Type_Struct>())
        return new StructValue(type->to<IR::Type_Struct>(), uninitialized, this);
    if (type->is<IR::Type_Header>())
        return new HeaderValue(type->to<IR::Type_Header>(), uninitialized, this);
    if (type->is<IR::Type_Stack>()) {
        auto st = type->to<IR::Type_Stack>();
        return new ArrayValue(st->baseType->to<IR::Type_Header>(), st->getSize(),
                              uninitialized, this);
    }
    if (type->is<IR::Type_Tuple>()) {
        auto tt = type->to<IR::Type_Tuple>();
        return new TupleValue(tt, uninitialized, this);
    }
    if (type->is<IR::Type_Extern>()) {
        auto te = type->to<IR::Type_Extern>();
        if (te->name.name == P4CoreLibrary::instance.packetIn.name) {
            return new PacketInValue(te);
        }
        return new ExternValue(te);
    }
    BUG("%1%: unexpected type", type);
}

bool ValueFactory::isFixedWidth(const IR::Type* type) const {
    type = typeMap->getType(type, true);
    if (type->is<IR::Type_Varbits>())
        return false;
    if (type->is<IR::Type_Extern>())
        return false;
    if (type->is<IR::Type_Stack>())
        return isFixedWidth(type->to<IR::Type_Stack>()->baseType);
    if (type->is<IR::Type_StructLike>()) {
        auto st = type->to<IR::Type_StructLike>();
        for (auto f : *st->fields)
            if (!isFixedWidth(f->type))
                return false;
        return true;
    }
    if (type->is<IR::Type_Tuple>()) {
        auto tt = type->to<IR::Type_Tuple>();
        for (auto f : *tt->components) {
            if (!isFixedWidth(f))
                return false;
        }
        return true;
    }
    return true;
}

unsigned ValueFactory::getWidth(const IR::Type* type) const {
    type = typeMap->getType(type, true);
    if (type->is<IR::Type_Bits>())
        return type->to<IR::Type_Bits>()->size;
    if (type->is<IR::Type_Boolean>())
        return 1;
    if (type->is<IR::Type_Union>()) {
        unsigned width = 0;
        for (auto f : *type->to<IR::Type_Union>()->fields)
            width = std::max(width, getWidth(f->type));
        return width;
    }
    if (type->is<IR::Type_StructLike>()) {
        unsigned width = 0;
        auto st = type->to<IR::Type_StructLike>();
        for (auto f : *st->fields)
            width += getWidth(f->type);
        return width;
    }
    if (type->is<IR::Type_Stack>()) {
        auto st = type->to<IR::Type_Stack>();
        auto bw = getWidth(st->baseType);
        return bw * st->getSize();
    }
    if (type->is<IR::Type_Tuple>()) {
        auto tt = type->to<IR::Type_Tuple>();
        unsigned width = 0;
        for (auto f : *tt->components)
            width += getWidth(f);
        return width;
    }
    return 0;
}

void AbstractBool::merge(const AbstractValue* other) {
    auto bo = other->to<AbstractBool>();
    state = mergeState(other->to<ScalarValue>()->state);
    if (state == ValueState::Constant && value != bo->value)
        state = ValueState::NotConstant;
}

void AbstractBool::assign(const AbstractValue* other) {
    BUG_CHECK(other->is<AbstractBool>(), "%1%: expected a bool", other);
    auto bo = other->to<AbstractBool>();
    state = bo->state;
    value = bo->value;
}

void AbstractInteger::merge(const AbstractValue* other) {
    BUG_CHECK(other->is<AbstractInteger>(), "%1%: expected an integer", other);
    auto io = other->to<AbstractInteger>();
    state = mergeState(io->state);
    if (state == ValueState::Constant && constant->value != io->constant->value) {
        state = ValueState::NotConstant;
        constant = nullptr;
    }
}

void AbstractInteger::assign(const AbstractValue* other) {
    if (other->is<ErrorValue>()) return;
    BUG_CHECK(other->is<AbstractInteger>(), "%1%: expected an integer", other);
    auto io = other->to<AbstractInteger>();
    state = io->state;
    constant = io->constant;
}

StructValue::StructValue(const IR::Type_StructLike* type,
                         bool uninitialized, const ValueFactory* factory) : type(type) {
    CHECK_NULL(type); CHECK_NULL(factory);
    for (auto f : *type->fields) {
        auto value = factory->create(f->type, uninitialized);
        fieldValue[f->name.name] = value;
    }
}

AbstractValue* StructValue::clone() const {
    auto result = new StructValue(type);
    for (auto f : fieldValue)
        result->fieldValue[f.first] = f.second->clone();
    return result;
}

void StructValue::assign(const AbstractValue* other) {
    if (other->is<ErrorValue>()) return;
    BUG_CHECK(other->is<StructValue>(), "%1%: expected a struct", other);
    auto sv = other->to<StructValue>();
    for (auto f : sv->fieldValue)
        fieldValue[f.first]->assign(f.second);
}

void StructValue::merge(const AbstractValue* other) {
    BUG_CHECK(other->is<StructValue>(), "%1%: expected a struct", other);
    auto sv = other->to<StructValue>();
    for (auto f : sv->fieldValue)
        fieldValue[f.first]->merge(f.second);
}

void StructValue::setAllUnknown() {
    for (auto f : *type->fields)
        fieldValue[f->name.name]->setAllUnknown();
}

HeaderValue::HeaderValue(const IR::Type_Header* type,
                         bool uninitialized,
                         const ValueFactory* factory) :
        StructValue(type, uninitialized, factory),
        valid(new AbstractBool(false)) {}

void HeaderValue::setValid(bool v) {
    if (!v)
        setAllUnknown();
    valid = new AbstractBool(v);
}

AbstractValue* HeaderValue::get(const IR::Node* node, cstring field) const {
    if (valid->isKnown() && !valid->value)
        return new StaticErrorValue(node, "Reading field from invalid header");
    return StructValue::get(node, field);
}

void HeaderValue::setAllUnknown() {
    StructValue::setAllUnknown();
    valid->setAllUnknown();
}

AbstractValue* HeaderValue::clone() const {
    auto result = new HeaderValue(type->to<IR::Type_Header>());
    for (auto f : fieldValue)
        result->fieldValue[f.first] = f.second->clone();
    result->valid = valid->clone()->to<AbstractBool>();
    return result;
}

void HeaderValue::assign(const AbstractValue* other) {
    if (other->is<ErrorValue>()) return;
    BUG_CHECK(other->is<HeaderValue>(), "%1%: expected a header", other);
    auto hv = other->to<HeaderValue>();
    for (auto f : hv->fieldValue)
        fieldValue[f.first]->assign(f.second);
    valid->assign(hv->valid);
}

void HeaderValue::merge(const AbstractValue* other) {
    BUG_CHECK(other->is<HeaderValue>(), "%1%: expected a header", other);
    auto hv = other->to<HeaderValue>();
    for (auto f : hv->fieldValue)
        fieldValue[f.first]->merge(f.second);
    valid->merge(hv->valid);
}

ArrayValue::ArrayValue(const IR::Type_Header* elemType, size_t size,
                       bool uninitialized, const ValueFactory* factory) : elemType(elemType) {
    for (unsigned i=0; i < size; i++) {
        auto elem = factory->create(elemType, uninitialized);
        BUG_CHECK(elem->is<HeaderValue>(), "%1%: expected a header", elem);
        values.push_back(elem->to<HeaderValue>());
    }
}

void ArrayValue::shift(int amount) {
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

AbstractValue* ArrayValue::next(const IR::Node* node) {
    for (unsigned i = 0; i < values.size(); i++) {
        auto v = values.at(i);
        if (v->valid->isUnknown() || v->valid->isUninitialized())
            return new AnyElement(this);
        if (!v->valid->value)
            return v;
    }
    return new ExceptionValue(node, P4::StandardExceptions::FullStack);
}

AbstractValue* ArrayValue::last(const IR::Node* node) {
    for (unsigned i = 0; i < values.size(); i++) {
        unsigned index = values.size() - i - 1;
        auto v = values.at(index);
        if (v->valid->isUnknown() || v->valid->isUninitialized())
            return new AnyElement(this);
        if (v->valid->value)
            return v;
    }
    return new ExceptionValue(node, P4::StandardExceptions::EmptyStack);
}

void ArrayValue::setAllUnknown() {
    for (unsigned i = 0; i < values.size(); i++)
        values.at(i)->setAllUnknown();
}

AbstractValue* ArrayValue::clone() const {
    auto result = new ArrayValue(elemType);
    for (unsigned i=0; i < values.size(); i++)
        result->values.push_back(get(nullptr, i)->clone()->to<HeaderValue>());
    return result;
}

void ArrayValue::assign(const AbstractValue* other) {
    if (other->is<ErrorValue>()) return;
    BUG_CHECK(other->is<ArrayValue>(), "%1%: expected an array", other);
    for (unsigned i=0; i < values.size(); i++)
        values.at(i)->assign(other->to<ArrayValue>()->get(nullptr, i));
}

void ArrayValue::merge(const AbstractValue* other) {
    BUG_CHECK(other->is<ArrayValue>(), "%1%: expected an array", other);
    for (unsigned i=0; i < values.size(); i++)
        values.at(i)->merge(other->to<ArrayValue>()->get(nullptr, i));
}

void AnyElement::merge(const AbstractValue* other) {
    for (unsigned i=0; i < parent->values.size(); i++)
        parent->values.at(i)->merge(other);
}

TupleValue::TupleValue(const IR::Type_Tuple* type, bool uninitialized,
                       const ValueFactory* factory) {
    for (auto t : *type->components) {
        auto v = factory->create(t, uninitialized);
        values.push_back(v);
    }
}

void TupleValue::setAllUnknown() {
    for (unsigned i = 0; i < values.size(); i++)
        values.at(i)->setAllUnknown();
}

AbstractValue* TupleValue::clone() const {
    auto result = new TupleValue();
    for (unsigned i=0; i < values.size(); i++)
        result->values.push_back(get(i)->clone());
    return result;
}

void TupleValue::merge(const AbstractValue* other) {
    BUG_CHECK(other->is<TupleValue>(), "%1%: expected a tuple value", other);
    auto tpl = other->to<TupleValue>();
    BUG_CHECK(values.size() == tpl->values.size(), "merging tuples with different sizes");
    for (unsigned i=0; i < values.size(); i++)
        values.at(i)->merge(tpl->get(i));
}

void PacketInValue::merge(const AbstractValue* other) {
    BUG_CHECK(other->is<PacketInValue>(), "%1%: merging with non-packet", other);
    auto pv = other->to<PacketInValue>();
    if (minimumStreamOffset > pv->minimumStreamOffset)
        minimumStreamOffset = pv->minimumStreamOffset;
}

VoidValue* VoidValue::instance = new VoidValue();

/*****************************************************************************************/

void ExpressionEvaluator::postorder(const IR::Operation_Binary* expression) {
    auto l = get(expression->left);
    if (l->is<ErrorValue>()) {
        set(expression, l);
        return;
    }
    auto r = get(expression->right);
    if (r->is<ErrorValue>()) {
        set(expression, l);
        return;
    }
    auto clone = expression->clone();
    BUG_CHECK(l->is<AbstractInteger>(), "%1%: expected an AbstractInteger", l);
    BUG_CHECK(r->is<AbstractInteger>(), "%1%: expected an AbstractInteger", r);
    auto li = l->to<AbstractInteger>();
    auto ri = r->to<AbstractInteger>();
    if (li->isUninitialized()) {
        auto result = new StaticErrorValue(expression->left, "Uninitialized");
        set(expression, result);
        return;
    } else if (ri->isUninitialized()) {
        auto result = new StaticErrorValue(expression->right, "Uninitialized");
        set(expression, result);
        return;
    } else if (!li->isUnknown() && !ri->isUnknown()) {
        clone->left = li->constant;
        clone->right = ri->constant;
        ConstantFolding cf(refMap, typeMap);
        auto result = expression->apply(cf);
        BUG_CHECK(result->is<IR::Constant>(), "%1%: expected a constant", result);
        set(expression, new AbstractInteger(result->to<IR::Constant>()));
        return;
    }
    set(expression, new AbstractInteger(ScalarValue::ValueState::NotConstant));
}

void ExpressionEvaluator::postorder(const IR::Operation_Unary* expression) {
    auto l = get(expression->expr);
    if (l->is<ErrorValue>()) {
        set(expression, l);
        return;
    }
    auto clone = expression->clone();
    BUG_CHECK(l->is<ScalarValue>(), "%1%: expected a scalar", l);
    auto sv = l->to<ScalarValue>();
    if (sv->isUninitialized()) {
        auto result = new StaticErrorValue(expression->expr, "Uninitialized");
        set(expression, result);
        return;
    }
    if (sv->isUnknown()) {
        set(expression, l);
        return;
    }

    if (l->is<AbstractInteger>()) {
        auto li = l->to<AbstractInteger>();
        clone->expr = li->constant;
        ConstantFolding cf(refMap, typeMap);
        auto result = expression->apply(cf);
        BUG_CHECK(result->is<IR::Constant>(), "%1%: expected a constant", result);
        set(expression, new AbstractInteger(result->to<IR::Constant>()));
        return;
    } else if (l->is<AbstractBool>()) {
        auto li = l->to<AbstractBool>();
        clone->expr = new IR::BoolLiteral(li->value);
        ConstantFolding cf(refMap, typeMap);
        auto result = expression->apply(cf);
        BUG_CHECK(result->is<IR::BoolLiteral>(), "%1%: expected a boolean", result);
        set(expression, new AbstractBool(result->to<IR::BoolLiteral>()));
        return;
    }
    BUG("%1%: unexpected type", l);
}

void ExpressionEvaluator::postorder(const IR::Constant* expression) {
    set(expression, new AbstractInteger(expression));
}

void ExpressionEvaluator::postorder(const IR::ListExpression* expression) {
    auto result = new TupleValue();
    for (auto e : *expression->components) {
        auto v = get(e);
        result->add(v);
    }
    set(expression, result);
}

void ExpressionEvaluator::postorder(const IR::BoolLiteral* expression) {
    set(expression, new AbstractBool(expression));
}

void ExpressionEvaluator::postorder(const IR::Operation_Relation* expression) {
    auto l = get(expression->left);
    if (l->is<ErrorValue>()) {
        set(expression, l);
        return;
    }
    auto r = get(expression->right);
    if (r->is<ErrorValue>()) {
        set(expression, l);
        return;
    }
    BUG_CHECK(l->is<ScalarValue>(), "%1%: expected a scalar", l);
    BUG_CHECK(r->is<ScalarValue>(), "%1%: expected a scalar", r);
    auto lv = l->to<ScalarValue>();
    auto rv = r->to<ScalarValue>();
    if (lv->isUninitialized()) {
        auto result = new StaticErrorValue(expression->left, "Uninitialized");
        set(expression, result);
        return;
    }
    if (rv->isUninitialized()) {
        auto result = new StaticErrorValue(expression->right, "Uninitialized");
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
    if (l->is<AbstractInteger>()) {
        BUG_CHECK(r->is<AbstractInteger>(), "%1%: expected an AbstractInteger");
        clone->left = l->to<AbstractInteger>()->constant;
        clone->right = r->to<AbstractInteger>()->constant;
        ConstantFolding cf(refMap, typeMap);
        auto result = expression->apply(cf);
        BUG_CHECK(result->is<IR::BoolLiteral>(), "%1%: expected a boolean", result);
        set(expression, new AbstractBool(result->to<IR::BoolLiteral>()));
        return;
    } else if (l->is<AbstractBool>()) {
        BUG_CHECK(r->is<AbstractBool>(), "%1%: expected an AbstractBool");
        clone->left = new IR::BoolLiteral(l->to<AbstractBool>()->value);
        clone->right = new IR::BoolLiteral(r->to<AbstractBool>()->value);
        ConstantFolding cf(refMap, typeMap);
        auto result = expression->apply(cf);
        BUG_CHECK(result->is<IR::BoolLiteral>(), "%1%: expected a boolean", result);
        set(expression, new AbstractBool(result->to<IR::BoolLiteral>()));
        return;
    }
    BUG("%1%: unexpected type", l);
}

void ExpressionEvaluator::postorder(const IR::Member* expression) {
    auto type = typeMap->getType(expression, true);
    if (type->is<IR::Type_MethodBase>()) {
        // not really void, but we can't do anything with this anyway
        set(expression, VoidValue::get());
        return;
    }
    auto l = get(expression->expr);
    if (l->is<ErrorValue>()) {
        set(expression, l);
        return;
    }
    auto basetype = typeMap->getType(expression->expr, true);
    if (basetype->is<IR::Type_Stack>()) {
        BUG_CHECK(l->is<ArrayValue>(), "%1%: expected an array", l);
        auto array = l->to<ArrayValue>();
        AbstractValue* v;
        if (expression->member.name == IR::Type_Stack::next) {
            v = array->next(expression);
            if (v->is<ErrorValue>()) {
                set(expression, v);
                return;
            }
        } else if (expression->member.name == IR::Type_Stack::last) {
            v = array->last(expression);
            if (v->is<ErrorValue>()) {
                set(expression, v);
                return;
            }
        } else {
            BUG("%1%: unexpected expression", expression);
        }
        set(expression, v);
    } else {
        BUG_CHECK(l->is<StructValue>(), "%1%: expected a struct", l);
        auto v = l->to<StructValue>()->get(expression, expression->member.name);
        set(expression, v);
    }
}

void ExpressionEvaluator::postorder(const IR::ArrayIndex* expression) {
    auto l = get(expression->left);
    auto r = get(expression->right);
    auto rv = r->to<ScalarValue>();
    auto lv = l->to<ArrayValue>();

    if (lv->is<ErrorValue>()) {
        set(expression, lv);
        return;
    }
    if (rv->is<ErrorValue>()) {
        set(expression, rv);
        return;
    }
    if (rv->isUninitialized() || rv->isUnknown()) {
        if (rv->isUninitialized()) {
            auto result = new StaticErrorValue(expression->right, "Uninitialized");
            set(expression, result);
            return;
        }
        auto v0 = new AnyElement(lv);
        set(expression, v0);
        return;
    }
    CHECK_NULL(lv);
    auto ix = r->to<AbstractInteger>();
    CHECK_NULL(ix);
    auto result = lv->get(expression, ix->constant->asInt());
    set(expression, result);
}

void ExpressionEvaluator::postorder(const IR::PathExpression* expression) {
    auto decl = refMap->getDeclaration(expression->path, true);
    auto result = valueMap->get(decl);
    set(expression, result);
}

void ExpressionEvaluator::postorder(const IR::MethodCallExpression* expression) {
    MethodCallDescription mcd(expression, refMap, typeMap);
    auto mi = mcd.instance;
    if (mi->is<BuiltInMethod>()) {
        auto bim = mi->to<BuiltInMethod>();
        auto base = get(bim->appliedTo);
        cstring name = bim->name.name;
        if (name == IR::Type_Header::setInvalid ||
            name == IR::Type_Header::setValid) {
            BUG_CHECK(base->is<HeaderValue>(), "%1%: expected a header", base);
            auto hv = base->to<HeaderValue>();
            hv->setValid(name == IR::Type_Header::setValid);
            set(expression, VoidValue::get());
            return;
        } else if (name == IR::Type_Stack::push_front ||
                   name == IR::Type_Stack::pop_front) {
            BUG_CHECK(base->is<ArrayValue>(), "%1%: expected an array", base);
            auto array = base->to<ArrayValue>();
            BUG_CHECK(expression->arguments->size() == 1, "%1%: not one argument?", expression);
            auto amount = get(expression->arguments->at(0));
            BUG_CHECK(amount->is<AbstractInteger>(), "%1%: expected an integer", amount);
            auto ac = amount->to<AbstractInteger>();
            if (ac->isUnknown()) {
                array->setAllUnknown();
                return;
            }
            BUG_CHECK(amount->is<AbstractInteger>(), "%1%: expected an integer", amount);
            int amt = amount->to<AbstractInteger>()->constant->asInt();
            if (name == IR::Type_Stack::pop_front)
                amt = -amt;
            array->shift(amt);
            set(expression, VoidValue::get());
            return;
        } else {
            // isValid()
            BUG_CHECK(base->is<HeaderValue>(), "%1%: expected a header", base);
            auto hv = base->to<HeaderValue>();
            auto v = hv->valid;
            set(expression, v);
            return;
        }
    }

    // For all methods: the in arguments are unchanged, and the out
    // arguments have an unknown value.
    for (auto p : *mcd.substitution.getParameters()) {
        if (p->direction == IR::Direction::Out || p->direction == IR::Direction::InOut) {
            auto expr = mcd.substitution.lookup(p);
            auto val = get(expr);
            val->setAllUnknown();
        }
    }

    if (mi->is<ExternMethod>()) {
        // There are some extern methods that we know something about
        auto em = mi->to<ExternMethod>();
        if (em->type->name.name == P4CoreLibrary::instance.packetIn.name) {
            // packet methods
            if (em->method->name.name == P4CoreLibrary::instance.packetIn.extract.name) {
                // We know that after an extract terminates the header argument
                // is always valid.
                auto arg0 = expression->arguments->at(0);
                auto hdr = get(arg0);
                if (!hdr->is<HeaderValue>())
                    error("%1%: expected a header", arg0);
                else
                    hdr->to<HeaderValue>()->setValid(true);
            }
        }
    }

    if (mi->methodType->returnType == nullptr ||
        mi->methodType->returnType->is<IR::Type_Void>()) {
        set(expression, VoidValue::get());
    } else {
        auto type = typeMap->getType(mi->methodType->returnType, true);
        auto res = factory->create(type, false);
        set(expression, res);
    }
}

AbstractValue* ExpressionEvaluator::evaluate(const IR::Expression* expression) {
    (void)expression->apply(*this);
    auto result = get(expression);
    return result;
}

}  // namespace P4
