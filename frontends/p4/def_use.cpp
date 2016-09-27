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

#include <boost/functional/hash.hpp>
#include "def_use.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/tableApply.h"
#include "parserCallGraph.h"

namespace P4 {

// name for header valid bit
static const cstring validFieldName = "$valid";
const LocationSet* LocationSet::empty = new LocationSet();
ProgramPoint ProgramPoint::uninitialized;

StorageLocation* StorageFactory::create(const IR::Type* type, cstring name) const {
    if (type->is<IR::Type_Bits>() ||
        type->is<IR::Type_Boolean>() ||
        type->is<IR::Type_Varbits>() ||
        type->is<IR::Type_Enum>() ||
        type->is<IR::Type_Error>())
        return new BaseLocation(type, name);
    if (type->is<IR::Type_StructLike>()) {
        type = typeMap->getType(type, true);  // get the canonical version
        auto st = type->to<IR::Type_StructLike>();
        auto result = new StructLocation(type, name);
        for (auto f : *st->fields) {
            auto sl = create(f->type, name + "." + f->name);
            result->addField(f->name, sl);
        }
        if (st->is<IR::Type_Header>()) {
            auto valid = create(IR::Type_Boolean::get(), name + "." + validFieldName);
            result->addField(validFieldName, valid);
        }
        return result;
    }
    if (type->is<IR::Type_Stack>()) {
        type = typeMap->getType(type, true);  // get the canonical version
        auto st = type->to<IR::Type_Stack>();
        auto result = new ArrayLocation(st, name);
        for (unsigned i = 0; i < st->getSize(); i++) {
            auto sl = create(st->elementType, name + "[" + Util::toString(i) + "]");
            result->addElement(i, sl);
        }
        return result;
    }
    return nullptr;
}

const LocationSet* StorageLocation::removeHeaders() const {
    auto result = new LocationSet();
    removeHeaders(result);
    return result;
}

void BaseLocation::removeHeaders(LocationSet* result) const
{ result->add(this); }

void StructLocation::addValidBits(LocationSet* result) const {
    if (type->is<IR::Type_Header>()) {
        addField(validFieldName, result);
    } else {
        for (auto f : fields())
            f->addValidBits(result);
    }
}

void StructLocation::addField(cstring field, LocationSet* result) const {
    auto f = ::get(fieldLocations, field);
    CHECK_NULL(f);
    result->add(f);
}

void StructLocation::removeHeaders(LocationSet* result) const {
    if (!type->is<IR::Type_Struct>()) return;
    for (auto f : fieldLocations)
        f.second->removeHeaders(result);
}

void ArrayLocation::addValidBits(LocationSet* result) const {
    for (auto e : *this)
        e->addValidBits(result);
}

void ArrayLocation::addElement(unsigned index, LocationSet* result) const {
    BUG_CHECK(index < elements.size(), "%1%: out of bounds index", index);
    result->add(elements.at(index));
}

const LocationSet* StorageLocation::getValidBits() const {
    auto result = new LocationSet();
    addValidBits(result);
    return result;
}

const LocationSet* LocationSet::join(const LocationSet* other) const {
    if (this == LocationSet::empty)
        return other;
    if (other == LocationSet::empty)
        return this;
    auto result = new LocationSet(locations);
    for (auto e : other->locations)
        result->add(e);
    return result;
}

const LocationSet* LocationSet::getField(cstring field) const {
    auto result = new LocationSet();
    for (auto l : locations) {
        if (l->is<StructLocation>()) {
            auto strct = l->to<StructLocation>();
            strct->addField(field, result);
        }
        else {
            BUG_CHECK(l->is<ArrayLocation>(), "%1%: expected an ArrayLocation", l);
            auto array = l->to<ArrayLocation>();
            for (auto f : *array)
                f->to<StructLocation>()->addField(field, result);
        }
    }
    return result;
}

const LocationSet* LocationSet::getValidField() const
{ return getField(validFieldName); }

const LocationSet* LocationSet::getIndex(unsigned index) const {
    auto result = new LocationSet();
    for (auto l : locations) {
        auto array = l->to<ArrayLocation>();
        array->addElement(index, result);
    }
    return result;
}

const LocationSet* LocationSet::allElements() const {
    auto result = new LocationSet();
    for (auto l : locations) {
        auto array = l->to<ArrayLocation>();
        for (auto e : *array)
            result->add(e);
    }
    return result;
}

const LocationSet* LocationSet::canonicalize() const {
    LocationSet* result = new LocationSet();
    for (auto e : locations)
        result->addCanonical(e);
    return result;
}

void LocationSet::addCanonical(const StorageLocation* location) {
    if (location->is<BaseLocation>()) {
        add(location);
    } else if (location->is<StructLocation>()) {
        for (auto f : location->to<StructLocation>()->fields())
            addCanonical(f);
    } else if (location->is<ArrayLocation>()) {
        for (auto e : *location->to<ArrayLocation>())
            addCanonical(e);
    } else {
        BUG("unexpected location");
    }
}

const ProgramPoints* ProgramPoints::merge(const ProgramPoints* with) const {
    auto result = new ProgramPoints(points);
    for (auto p : with->points)
        result->points.emplace(p);
    return result;
}

ProgramPoint::ProgramPoint(const ProgramPoint &context, const IR::Node* node) {
    for (auto e : context.stack)
        stack.push_back(e);
    stack.push_back(node);
}

bool ProgramPoint::operator==(const ProgramPoint& other) const {
    if (stack.size() != other.stack.size())
        return false;
    for (unsigned i=0; i < stack.size(); i++)
        if (stack.at(i) != other.stack.at(i))
            return false;
    return true;
}

std::size_t ProgramPoint::hash() const {
    std::size_t result = 0;
    boost::hash_range(result, stack.begin(), stack.end());
    return result;
}

bool ProgramPoints::operator==(const ProgramPoints& other) const {
    if (points.size() != other.points.size())
        return false;
    for (auto p : points)
        if (other.points.find(p) == other.points.end())
            return false;
    return true;
}

Definitions* Definitions::join(const Definitions* other) const {
    auto result = new Definitions();
    for (auto d : other->definitions) {
        auto loc = d.first;
        auto defs = d.second;
        auto current = ::get(definitions, loc);
        if (current != nullptr) {
            auto merged = current->merge(defs);
            result->definitions.emplace(loc, merged);
        } else {
            result->definitions.emplace(loc, defs);
        }
    }
    for (auto d : definitions) {
        auto loc = d.first;
        auto defs = d.second;
        auto current = ::get(other->definitions, loc);
        if (current == nullptr)
            result->definitions.emplace(loc, defs);
        // otherwise have have already done it in the loop above
    }
    return result;
}

void Definitions::set(const StorageLocation* location, const ProgramPoints* point) {
    LocationSet locset;
    locset.addCanonical(location);
    for (auto sl : locset)
        definitions[sl->to<BaseLocation>()] = point;
}

void Definitions::set(const LocationSet* locations, const ProgramPoints* point) {
    for (auto sl : *locations->canonicalize())
        definitions[sl->to<BaseLocation>()] = point;
}

const ProgramPoints* Definitions::get(const LocationSet* locations) const {
    const ProgramPoints* result = new ProgramPoints();
    for (auto sl : *locations->canonicalize()) {
        auto points = get(sl->to<BaseLocation>());
        result = result->merge(points);
    }
    return result;
}

Definitions* Definitions::writes(ProgramPoint point, const LocationSet* locations) const {
    auto result = new Definitions(*this);
    auto points = new ProgramPoints();
    points->add(point);
    auto canon = locations->canonicalize();
    for (auto l : *canon)
        result->set(l->to<BaseLocation>(), points);
    return result;
}

bool Definitions::operator==(const Definitions& other) const {
    if (definitions.size() != other.definitions.size())
        return false;
    for (auto d : definitions) {
        auto od = ::get(other.definitions, d.first);
        if (od == nullptr)
            return false;
        if (!d.second->operator==(*od))
            return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// ComputeWriteSet implementation

// This assumes that all variable declarations have been pushed to the top
void ComputeWriteSet::initialize(const IR::IApply* block,
                                 const IR::IndexedVector<IR::Declaration>* locals,
                                 ProgramPoint startPoint) {
    auto defs = new Definitions();
    auto startPoints = new ProgramPoints(startPoint);
    auto uninit = new ProgramPoints(ProgramPoint::uninitialized);

    for (auto p : *block->getApplyMethodType()->parameters->parameters) {
        StorageLocation* loc = definitions->storageMap->add(p);
        if (loc == nullptr)
            continue;
        if (p->direction == IR::Direction::In ||
            p->direction == IR::Direction::InOut)
            defs->set(loc, startPoints);
        else if (p->direction == IR::Direction::Out)
            defs->set(loc, uninit);
        auto valid = loc->getValidBits();
        defs->set(valid, startPoints);
    }
    for (auto d : *locals) {
        if (d->is<IR::Declaration_Variable>()) {
            StorageLocation* loc = definitions->storageMap->add(d);
            if (loc != nullptr) {
                defs->set(loc, uninit);
                auto valid = loc->getValidBits();
                defs->set(valid, startPoints);
            }
        }
    }
    definitions->set(startPoint, defs);
    currentDefinitions = defs;
}

Definitions* ComputeWriteSet::getDefinitionsAfter(const IR::ParserState* state) {
    ProgramPoint last;
    if (state->components->size() == 0)
        last = ProgramPoint(state);
    else
        last = ProgramPoint(ProgramPoint(state), state->components->back());
    return definitions->get(last);
}

// if node is nullptr, use getOriginal().
ProgramPoint ComputeWriteSet::getProgramPoint(const IR::Node* node) const {
    if (node == nullptr) {
        node = getOriginal<IR::Statement>();
        CHECK_NULL(node);
    }
    return ProgramPoint(callingContext, node);
}

// set the currentDefinitions after executing node
bool ComputeWriteSet::setDefinitions(Definitions* defs, const IR::Node* node) {
    CHECK_NULL(defs);
    currentDefinitions = defs;
    auto point = getProgramPoint(node);
    definitions->set(point, currentDefinitions);
    LOG3("Definitions at " << point << " are " << std::endl << defs);
    return false;  // always returns false
}

////////////////////////////
//// visitor implementation

/// For expressions we maintain the write-set in the writes std::map

bool ComputeWriteSet::preorder(const IR::Literal* expression) {
    set(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::Slice* expression) {
    visit(expression->e0);
    auto base = get(expression->e0);
    if (lhs)
        set(expression, base);
    else
        set(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::TypeNameExpression* expression) {
    set(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::PathExpression* expression) {
    if (!lhs) {
        set(expression, LocationSet::empty);
        return false;
    }
    auto decl = storageMap->refMap->getDeclaration(expression->path, true);
    auto storage = storageMap->getStorage(decl);
    const LocationSet* result;
    if (storage != nullptr)
        result = new LocationSet(storage);
    else
        result = LocationSet::empty;
    set(expression, result);
    return false;
}

bool ComputeWriteSet::preorder(const IR::Member* expression) {
    visit(expression->expr);
    if (!lhs) {
        set(expression, LocationSet::empty);
        return false;
    }
    auto type = storageMap->typeMap->getType(expression, true);
    if (type->is<IR::Type_Method>())
        return false;
    if (TableApplySolver::isHit(expression, storageMap->refMap, storageMap->typeMap) ||
        TableApplySolver::isActionRun(expression, storageMap->refMap, storageMap->typeMap))
        return false;
    auto storage = get(expression->expr);

    auto basetype = storageMap->typeMap->getType(expression->expr, true);
    if (basetype->is<IR::Type_Stack>()) {
        if (expression->member.name == IR::Type_Stack::next ||
            expression->member.name == IR::Type_Stack::last) {
            set(expression, storage);
            return false;
        }
    }

    auto fields = storage->getField(expression->member);
    set(expression, fields);
    return false;
}

bool ComputeWriteSet::preorder(const IR::ArrayIndex* expression) {
    visit(expression->left);
    auto save = lhs;
    lhs = false;
    visit(expression->right);
    lhs = save;
    auto storage = get(expression->left);
    if (expression->right->is<IR::Constant>()) {
        if (!lhs) {
            set(expression, LocationSet::empty);
        } else {
            auto cst = expression->right->to<IR::Constant>();
            auto index = cst->asInt();
            auto result = storage->getIndex(index);
            set(expression, result);
        }
    } else {
        set(expression, storage->allElements());
    }
    return false;
}

bool ComputeWriteSet::preorder(const IR::Operation_Binary* expression) {
    visit(expression->left);
    visit(expression->right);
    auto l = get(expression->left);
    auto r = get(expression->right);
    auto result = l->join(r);
    set(expression, result);
    return false;
}

bool ComputeWriteSet::preorder(const IR::Mux* expression) {
    visit(expression->e0);
    visit(expression->e1);
    visit(expression->e2);
    auto e0 = get(expression->e0);
    auto e1 = get(expression->e1);
    auto e2 = get(expression->e2);
    auto result = e0->join(e1)->join(e2);
    set(expression, result);
    return false;
}

bool ComputeWriteSet::preorder(const IR::SelectExpression* expression) {
    BUG_CHECK(!lhs, "%1%: unexpected in lhs", expression);
    visit(expression->select);
    visit(&expression->selectCases);
    auto l = get(expression->select);
    for (auto c : expression->selectCases) {
        auto s = get(c->keyset);
        l = l->join(s);
    }
    set(expression, l);
    return false;
}

bool ComputeWriteSet::preorder(const IR::ListExpression* expression) {
    visit(expression->components);
    auto l = LocationSet::empty;
    for (auto c : *expression->components) {
        auto cl = get(c);
        l = l->join(cl);
    }
    set(expression, l);
    return false;
}

bool ComputeWriteSet::preorder(const IR::Operation_Unary* expression) {
    visit(expression->expr);
    auto result = get(expression->expr);
    set(expression, result);
    return false;
}

bool ComputeWriteSet::preorder(const IR::MethodCallExpression* expression) {
    LOG2("Visiting " << dbp(expression));
    visit(expression->method);
    MethodCallDescription mcd(expression, storageMap->refMap, storageMap->typeMap);
    if (mcd.instance->is<BuiltInMethod>()) {
        auto bim = mcd.instance->to<BuiltInMethod>();
        auto base = get(bim->appliedTo);
        cstring name = bim->name.name;
        if (name == IR::Type_Header::setInvalid) {
            // modifies all fields of the header!
            set(expression, base);
            return false;
        } else if (name == IR::Type_Header::setValid) {
            // modifies only the valid field
            auto v = base->getField(validFieldName);
            set(expression, v);
            return false;
        } else if (name == IR::Type_Stack::push_front ||
                   name == IR::Type_Stack::pop_front) {
            set(expression, base);
            return false;
        } else {
            BUG_CHECK(name == IR::Type_Header::isValid,
                      "%1%: unexpected method", bim->name);
            set(expression, LocationSet::empty);
            return false;
        }
    }

    // Symbolically call some methods (actions and tables)
    const IR::Node* callee = nullptr;
    if (mcd.instance->is<ActionCall>()) {
        auto action = mcd.instance->to<ActionCall>()->action;
        callee = action;
    } else if (mcd.instance->isApply()) {
        auto am = mcd.instance->to<ApplyMethod>();
        if (am->isTableApply()) {
            auto table = am->object->to<IR::P4Table>();
            callee = table;
        }
    }
    if (callee != nullptr) {
        LOG1("Analyzing " << callee);
        ProgramPoint pt(callingContext, expression);
        ComputeWriteSet cw(this, pt, currentDefinitions);
        (void)callee->apply(cw);
        currentDefinitions = definitions->get(pt);
    }

    // For all other methods we act conservatively:
    // out/inout arguments are written
    auto result = LocationSet::empty;
    for (auto p : *mcd.substitution.getParameters()) {
        auto expr = mcd.substitution.lookup(p);
        bool save = lhs;
        // pretend we are on the lhs
        lhs = true;
        visit(expr);
        lhs = save;
        if (p->direction == IR::Direction::Out || p->direction == IR::Direction::InOut) {
            auto val = get(expr);
            result = result->join(val);
        }
    }
    set(expression, result);
    return false;
}

//////////////////////////
/// Statements and other control structures

bool ComputeWriteSet::preorder(const IR::ParserState* state) {
    for (auto stat : *state->components)
        visit(stat);
    return false;
}

// Symbolic execution of the parser
bool ComputeWriteSet::preorder(const IR::P4Parser* parser) {
    LOG1("Visiting " << dbp(parser));
    auto startState = parser->getDeclByName(IR::ParserState::start)->to<IR::ParserState>();
    auto startPoint = ProgramPoint(startState);
    initialize(parser, parser->parserLocals, startPoint);

    ParserCallGraph transitions("transitions");
    ComputeParserCG pcg(storageMap->refMap, &transitions);

    (void)parser->apply(pcg);
    std::set<const IR::ParserState*> toRun;  // worklist
    toRun.emplace(startState);

    while (!toRun.empty()) {
        auto state = *toRun.begin();
        toRun.erase(state);
        LOG1("Traversing " << state);

        // We need a new visitor to visit the state,
        // but we use the same data structures
        ProgramPoint pt(state);
        ComputeWriteSet cws(this, pt, currentDefinitions);
        (void)state->apply(cws);

        ProgramPoint sp(state);
        auto after = getDefinitionsAfter(state);
        auto next = transitions.getCallees(state);
        for (auto n : *next) {
            ProgramPoint pt(n);
            auto defs = definitions->get(pt, true);
            auto newdefs = defs->join(after);
            if (!(*defs == *newdefs)) {
                // Only run once more if there are any changes
                setDefinitions(newdefs, n);
                toRun.emplace(n);
            }
        }
    }
    return false;
}

bool ComputeWriteSet::preorder(const IR::P4Control* control) {
    LOG1("Visiting " << dbp(control));
    auto startPoint = ProgramPoint(control);
    initialize(control, control->controlLocals, startPoint);
    exitDefinitions = new Definitions();
    returnedDefinitions = new Definitions();
    visit(control->body);
    auto returned = currentDefinitions->join(returnedDefinitions);
    auto exited = returned->join(exitDefinitions);
    return setDefinitions(exited, control->body);
}

bool ComputeWriteSet::preorder(const IR::IfStatement* statement) {
    visit(statement->condition);
    auto cond = get(statement->condition);
    // defs are the definitions after evaluating the condition
    auto defs = currentDefinitions->writes(getProgramPoint(), cond);
    (void)setDefinitions(defs, statement->condition);
    visit(statement->ifTrue);
    auto result = currentDefinitions;
    currentDefinitions = defs;
    if (statement->ifFalse != nullptr)
        visit(statement->ifFalse);
    result = result->join(currentDefinitions);
    return setDefinitions(result);
}

bool ComputeWriteSet::preorder(const IR::BlockStatement* statement) {
    visit(statement->components);
    return setDefinitions(currentDefinitions);
}

bool ComputeWriteSet::preorder(const IR::ReturnStatement*) {
    returnedDefinitions = returnedDefinitions->join(currentDefinitions);
    LOG3("Return definitions " << returnedDefinitions);
    return setDefinitions(new Definitions());
}

bool ComputeWriteSet::preorder(const IR::ExitStatement*) {
    exitDefinitions = exitDefinitions->join(currentDefinitions);
    LOG3("Exit definitions " << exitDefinitions);
    return setDefinitions(new Definitions());
}

bool ComputeWriteSet::preorder(const IR::EmptyStatement*) {
    return setDefinitions(currentDefinitions);
}

bool ComputeWriteSet::preorder(const IR::AssignmentStatement* statement) {
    lhs = true;
    visit(statement->left);
    lhs = false;
    visit(statement->right);
    const LocationSet* locs;
    auto l = get(statement->left);
    auto r = get(statement->right);
    locs = l->join(r);
    auto defs = currentDefinitions->writes(getProgramPoint(), locs);
    return setDefinitions(defs);
}

bool ComputeWriteSet::preorder(const IR::SwitchStatement* statement) {
    visit(statement->expression);
    auto locs = get(statement->expression);
    auto defs = currentDefinitions->writes(getProgramPoint(), locs);
    (void)setDefinitions(defs, statement->expression);
    auto save = currentDefinitions;
    auto result = currentDefinitions;
    bool seenDefault = false;
    for (auto s : statement->cases) {
        if (s->label->is<IR::DefaultExpression>())
            seenDefault = true;
        visit(s->statement);
        result = result->join(currentDefinitions);
    }
    auto table = TableApplySolver::isActionRun(
        statement->expression, storageMap->refMap, storageMap->typeMap);
    CHECK_NULL(table);
    auto al = table->getActionList();
    bool allCases = statement->cases.size() == al->size();
    if (!seenDefault && !allCases)
        // no case may have been executed
        result = result->join(save);
    return setDefinitions(result);
}

bool ComputeWriteSet::preorder(const IR::P4Action* action) {
    // TODO: call initialize here too to analyze action locals
    LOG1("Visiting " << dbp(action));
    returnedDefinitions = new Definitions();
    visit(action->body);
    currentDefinitions = currentDefinitions->join(returnedDefinitions);
    definitions->set(callingContext, currentDefinitions);
    return false;
}

bool ComputeWriteSet::preorder(const IR::P4Table* table) {
    LOG1("Visiting " << dbp(table));
    // non-deterministic call of one of the actions in the table
    auto after = currentDefinitions;
    auto actions = table->getActionList();
    for (auto ale : *actions->actionList) {
        const IR::P4Action *action;
        if (ale->expression->is<IR::PathExpression>()) {
            auto act = storageMap->refMap->getDeclaration(ale->expression->to<IR::PathExpression>()->path);
            BUG_CHECK(act->is<IR::P4Action>(), "%1%: expected an action", ale->expression);
            action = act->to<IR::P4Action>();
        } else if (ale->expression->is<IR::MethodCallExpression>()) {
            auto mi = MethodInstance::resolve(ale->expression->to<IR::MethodCallExpression>(),
                                              storageMap->refMap, storageMap->typeMap);
            BUG_CHECK(mi->is<ActionCall>(), "%1%: expected an action call", ale->expression);
            action = mi->to<ActionCall>()->action;
        } else {
            BUG("%1%: Unexpected", ale->expression);
        }

        ProgramPoint pt(callingContext, table);
        ComputeWriteSet cw(this, pt, currentDefinitions);
        (void)action->apply(cw);
        currentDefinitions = definitions->get(pt);
        after = after->join(currentDefinitions);
    }
    definitions->set(callingContext, currentDefinitions);
    return false;
}

bool ComputeWriteSet::preorder(const IR::MethodCallStatement* statement) {
    lhs = false;
    visit(statement->methodCall);
    auto locs = get(statement->methodCall);
    auto defs = currentDefinitions->writes(getProgramPoint(), locs);
    return setDefinitions(defs);
}

}  // namespace P4
