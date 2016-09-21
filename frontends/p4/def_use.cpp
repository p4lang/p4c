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

#include "def_use.h"
#include "frontends/p4/methodInstance.h"
#include "parserCallGraph.h"
#include <boost/functional/hash.hpp>

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

void StructLocation::addValidBits(LocationSet* result) const {
    if (type->is<IR::Type_Header>()) {
        result->add(getField(validFieldName));
    } else {
        for (auto f : fields())
            f->addValidBits(result);
    }
}

void ArrayLocation::addValidBits(LocationSet* result) const {
    for (auto e : *this)
        e->addValidBits(result);
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
        auto strct = l->to<StructLocation>();
        auto f = strct->getField(field);
        result->add(f);
    }
    return result;
}

const LocationSet* LocationSet::getValidField() const
{ return getField(validFieldName); }

const LocationSet* LocationSet::getIndex(unsigned index) const {
    auto result = new LocationSet();
    for (auto l : locations) {
        auto array = l->to<ArrayLocation>();
        auto elem = array->getElement(index);
        result->add(elem);
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

namespace {
// Computes all locations written to by an expression.
// Expressions can write something by being on the lhs of an assignment,
// or by calling methods with out parameters.
class WriteSet : public Inspector {
    const StorageMap* map;
    bool lhs;    // if true the expression appears on the lhs of an assignment
    std::map<const IR::Expression*, const LocationSet*> locations;

    const LocationSet* get(const IR::Expression* expression) const {
        auto result = ::get(locations, expression);
        CHECK_NULL(result);
        return result;
    }
    void set(const IR::Expression* expression, const LocationSet* loc) {
        CHECK_NULL(expression);
        CHECK_NULL(loc);
        locations.emplace(expression, loc);
    }
    bool preorder(const IR::Literal* expression) override
    { set(expression, LocationSet::empty); return false; }
    bool preorder(const IR::Slice* expression) override {
        visit(expression->e0);
        auto base = get(expression->e0);
        if (lhs)
            set(expression, base);
        else
            set(expression, LocationSet::empty);
        return false;
    }
    bool preorder(const IR::TypeNameExpression* expression) override
    { set(expression, LocationSet::empty); return false; }
    bool preorder(const IR::PathExpression* expression) override {
        if (!lhs) {
            set(expression, LocationSet::empty);
            return false;
        }
        auto decl = map->refMap->getDeclaration(expression->path, true);
        auto storage = map->getStorage(decl);
        const LocationSet* result;
        if (storage != nullptr)
            result = new LocationSet(storage);
        else
            result = LocationSet::empty;
        set(expression, result);
        return false;
    }
    bool preorder(const IR::Member* expression) override {
        visit(expression->expr);
        if (!lhs) {
            set(expression, LocationSet::empty);
            return false;
        }
        auto type = map->typeMap->getType(expression, true);
        if (type->is<IR::Type_Method>())
            return false;
        auto storage = get(expression->expr);

        auto basetype = map->typeMap->getType(expression->expr, true);
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
    bool preorder(const IR::ArrayIndex* expression) override {
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
    bool preorder(const IR::Operation_Binary* expression) override {
        visit(expression->left);
        visit(expression->right);
        auto l = get(expression->left);
        auto r = get(expression->right);
        auto result = l->join(r);
        set(expression, result);
        return false;
    }
    bool preorder(const IR::Mux* expression) override {
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
    bool preorder(const IR::SelectExpression* expression) override {
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
    bool preorder(const IR::ListExpression* expression) override {
        visit(expression->components);
        auto l = LocationSet::empty;
        for (auto c : *expression->components) {
            auto cl = get(c);
            l = l->join(cl);
        }
        set(expression, l);
        return false;
    }
    bool preorder(const IR::Operation_Unary* expression) override {
        visit(expression->expr);
        auto result = get(expression->expr);
        set(expression, result);
        return false;
    }
    bool preorder(const IR::MethodCallExpression* expression) override {
        visit(expression->method);
        MethodCallDescription mcd(expression, map->refMap, map->typeMap);
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
 public:
    WriteSet(const StorageMap* map, bool lhs) :
            map(map), lhs(lhs)
    { CHECK_NULL(map); }
    const LocationSet* getLocations(const IR::Expression* expression) {
        expression->apply(*this);
        return get(expression);
    }
};
}  // namespace

const LocationSet* StorageMap::writes(const IR::Expression* expression, bool lhs) const {
    WriteSet ws(this, lhs);
    return ws.getLocations(expression);
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

// This assumes that all variable declarations have been pushed to the top
void ComputeDefUse::initialize(const IR::P4Parser* parser) {
    auto defs = new Definitions();
    auto startState = parser->getDeclByName(IR::ParserState::start)->getNode();
    startPoint = ProgramPoint(startState);
    auto startPoints = new ProgramPoints(startPoint);
    auto uninit = new ProgramPoints(ProgramPoint::uninitialized);

    for (auto p : *parser->type->applyParams->parameters) {
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
    for (auto d : *parser->parserLocals) {
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
    LOG1(parser << " initial definitions " << defs);
}

Definitions* ComputeDefUse::visitParserStatement(const IR::StatOrDecl* stat, Definitions* previous) {
    if (!stat->is<IR::Statement>())
        return previous;

    ProgramPoint point(stat);
    const LocationSet* locs;
    if (stat->is<IR::AssignmentStatement>()) {
        auto assign = stat->to<IR::AssignmentStatement>();
        auto l = definitions->storageMap->writes(assign->left, true);
        auto r = definitions->storageMap->writes(assign->right, false);
        locs = l->join(r);
    } else if (stat->is<IR::MethodCallStatement>()) {
        locs = definitions->storageMap->writes(stat->to<IR::MethodCallStatement>()->methodCall, false);
    } else if (stat->is<IR::BlockStatement>()) {
        auto block = stat->to<IR::BlockStatement>();
        auto defs = previous;
        for (auto s : *block->components)
            defs = visitParserStatement(s, defs);
        LOG3("Setting definitions after " << point << " to:" << std::endl << defs);
        definitions->set(point, defs);
        return defs;
    } else {
        BUG("%1%: Unexpected statement", stat);
    }
    // TODO: handle conditionals
    auto result = previous->writes(point, locs);
    LOG3("Setting definitions after " << point << " to:" << std::endl << result);
    definitions->set(point, result);
    return result;
}

Definitions* ComputeDefUse::visitParserState(const IR::ParserState* state) {
    ProgramPoint sp(state);
    auto defs = definitions->get(sp);
    for (auto stat : *state->components)
        defs = visitParserStatement(stat, defs);
    return defs;
}

Definitions* ComputeDefUse::getDefinitionsAfter(const IR::ParserState* state) {
    ProgramPoint last;
    if (state->components->size() == 0)
        last = ProgramPoint(state);
    else
        last = ProgramPoint(state->components->back());
    return definitions->get(last);
}

bool ComputeDefUse::preorder(const IR::P4Parser* parser) {
    ParserCallGraph transitions("transitions");
    ComputeParserCG pcg(refMap, &transitions);
    initialize(parser);

    (void)parser->apply(pcg);
    auto start = parser->getDeclByName(IR::ParserState::start)->to<IR::ParserState>();
    std::set<const IR::ParserState*> toRun;  // worklist
    toRun.emplace(start);

    while (!toRun.empty()) {
        auto state = *toRun.begin();
        toRun.erase(state);
        LOG1("Traversing " << state);

        ProgramPoint sp(state);
        auto after = visitParserState(state);
        auto next = transitions.getCallees(state);
        for (auto n : *next) {
            ProgramPoint pt(n);
            auto defs = definitions->get(pt);
            auto newdefs = defs->join(after);
            if (!(*defs == *newdefs)) {
                definitions->set(pt, newdefs);
                toRun.emplace(n);
            }
        }
    }
    return false;
}

/////////////// Control-def-use

// This assumes that all variable declarations have been pushed to the top
void ComputeDefUse::initialize(const IR::P4Control* control) {
    auto defs = new Definitions();
    startPoint = ProgramPoint(control);
    auto startPoints = new ProgramPoints(startPoint);
    auto uninit = new ProgramPoints(ProgramPoint::uninitialized);

    for (auto p : *control->type->applyParams->parameters) {
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
    for (auto d : *control->controlLocals) {
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
    LOG1(control << " initial definitions " << defs);
    currentDefinitions = defs;
}

bool ComputeDefUse::preorder(const IR::P4Control* control) {
    initialize(control);
    visit(control->body);
    return false;  // prune
}

// if node is nullptr, use getOriginal().
ProgramPoint ComputeDefUse::getProgramPoint(const IR::Node* node) const {
    if (node == nullptr) {
        node = getOriginal<IR::Statement>();
        CHECK_NULL(node);
    }
    return ProgramPoint(callingContext, node);
}

// set the currentDefinitions after executing node
bool ComputeDefUse::setDefinitions(Definitions* defs, const IR::Node* node) {
    CHECK_NULL(defs);
    currentDefinitions = defs;
    auto point = getProgramPoint(node);
    definitions->set(point, currentDefinitions);
    LOG3("Setting definitions after " << point << " to:" << std::endl << currentDefinitions);
    return false;  // always returns false
}

bool ComputeDefUse::preorder(const IR::IfStatement* statement) {
    auto cond = definitions->storageMap->writes(statement->condition, false);
    // defs are the definitions after evaluating the condition
    auto defs = currentDefinitions->writes(getProgramPoint(), cond);
    (void)setDefinitions(defs, statement->condition);
    visit(statement->ifTrue);
    auto result = currentDefinitions;
    if (statement->ifFalse != nullptr) {
        currentDefinitions = defs;
        visit(statement->ifFalse);
        result = result->join(currentDefinitions);
    }
    return setDefinitions(result);
}

bool ComputeDefUse::preorder(const IR::BlockStatement* statement) {
    visit(statement->components);
    return setDefinitions(currentDefinitions);
}

bool ComputeDefUse::preorder(const IR::ReturnStatement*) {
    auto defs = new Definitions();
    return setDefinitions(defs);
}

bool ComputeDefUse::preorder(const IR::ExitStatement*) {
    auto defs = new Definitions();
    return setDefinitions(defs);
}

bool ComputeDefUse::preorder(const IR::EmptyStatement*) {
    return setDefinitions(currentDefinitions);
}

bool ComputeDefUse::preorder(const IR::AssignmentStatement* statement) {
    const LocationSet* locs;
    auto l = definitions->storageMap->writes(statement->left, true);
    auto r = definitions->storageMap->writes(statement->right, false);
    locs = l->join(r);
    auto defs = currentDefinitions->writes(getProgramPoint(), locs);
    return setDefinitions(defs);
}

bool ComputeDefUse::preorder(const IR::SwitchStatement* statement) {
    // TODO
    return false;
}

bool ComputeDefUse::preorder(const IR::MethodCallStatement* statement) {
    // TODO
    return false;
}

}  // namespace P4
