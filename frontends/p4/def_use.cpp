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
#include <boost/functional/hash.hpp>

namespace P4 {

// name for header valid bit
static const cstring validFieldName = "$valid";
const LocationSet* empty = new LocationSet();

StorageLocation* StorageFactory::create(const IR::Type* type) const {
    type = typeMap->getType(type, true);
    if (type->is<IR::Type_Bits>() ||
        type->is<IR::Type_Boolean>() ||
        type->is<IR::Type_Varbits>() ||
        type->is<IR::Type_Enum>() ||
        type->is<IR::Type_Error>())
        return new BaseLocation(type);
    if (type->is<IR::Type_StructLike>()) {
        auto st = type->to<IR::Type_StructLike>();
        auto result = new StructLocation(type);
        for (auto f : *st->fields) {
            auto sl = create(f->type);
            result->addField(f->name, sl);
        }
        if (st->is<IR::Type_Header>()) {
            auto valid = create(IR::Type_Boolean::get());
            result->addField(validFieldName, valid);
        }
        return result;
    }
    if (type->is<IR::Type_Stack>()) {
        auto st = type->to<IR::Type_Stack>();
        auto result = new ArrayLocation(st);
        for (unsigned i = 0; i < st->getSize(); i++) {
            auto sl = create(st->elementType);
            result->addElement(i, sl);
        }
        return result;
    }
    return nullptr;
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

const LocationSet* LocationSet::getIndex(unsigned index) const {
    auto result = new LocationSet();
    for (auto l : locations) {
        auto array = l->to<ArrayLocation>();
        auto elem = array->getElement(index);
        result->add(elem);
    }
    return result;
}

const LocationSet* LocationSet::canonicalize() const {
    LocationSet* result = new LocationSet();
    for (auto e : locations)
        result->addCanonical(e);
    return result;
}

void LocationSet::addCanonical(StorageLocation* location) {
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
// This assumes that expressions have been simplified, and that no side-effects occur
// in expressions in the LHS of an assignment.
class Locations : public Inspector {
    const StorageMap* map;
    bool reads;  // if true compute read set, else compute write set
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
    void postorder(const IR::Literal* expression)
    { set(expression, LocationSet::empty); }
    void postorder(const IR::Slice* expression) {
        auto base = get(expression->e0);
        if (reads) {
            set(expression, base);
        } else {
            if (lhs)
                set(expression, base);
            else
                set(expression, LocationSet::empty);
        }
    }
    void postorder(const IR::TypeNameExpression* expression)
    { set(expression, LocationSet::empty); }
    void postorder(const IR::PathExpression* expression) {
        auto decl = map->refMap->getDeclaration(expression->path, true);
        auto storage = map->getStorage(decl);
        auto result = new LocationSet();
        result->add(storage);
        set(expression, result);
    }
    void postorder(const IR::Member* expression) {
        auto storage = get(expression->expr);
        auto fields = storage->getField(expression->member);
        set(expression, fields);
    }
    bool preorder(const IR::ArrayIndex* expression) {
        visit(expression->left);
        auto save = lhs;
        lhs = false;
        visit(expression->right);
        lhs = save;
        return false;  // prune()
    }
    void postorder(const IR::ArrayIndex* expression) {
        auto storage = get(expression->left);
        if (expression->right->is<IR::Constant>()) {
            auto cst = expression->right->to<IR::Constant>();
            auto index = cst->asInt();
            auto result = storage->getIndex(index);
            set(expression, result);
        } else {
            // we model the update with an unknown index as a read
            // (and write if in LHS) to the whole array
            set(expression, storage);
        }
    }
    void postorder(const IR::Operation_Binary* expression) {
        BUG_CHECK(reads, "%1%: unexpected expression", expression);
        auto l = get(expression->left);
        auto r = get(expression->right);
        auto result = l->join(r);
        set(expression, result);
    }
    void postorder(const IR::Mux* expression) {
        BUG_CHECK(reads, "%1%: unexpected expression", expression);
        auto e0 = get(expression->e0);
        auto e1 = get(expression->e1);
        auto e2 = get(expression->e2);
        auto result = e0->join(e1)->join(e2);
        set(expression, result);
    }
    void postorder(const IR::SelectExpression* expression) {
        auto l = get(expression->select);
        for (auto c : expression->selectCases) {
            auto s = get(c->keyset);
            l = l->join(s);
        }
        set(expression, l);
    }
    void postorder(const IR::ListExpression* expression) {
        BUG_CHECK(reads, "%1%: unexpected expression", expression);
        auto l = LocationSet::empty;
        for (auto c : *expression->components) {
            auto cl = get(c);
            l = l->join(cl);
        }
        set(expression, l);
    }
    void postorder(const IR::Operation_Unary* expression) {
        BUG_CHECK(reads, "%1%: unexpected expression", expression);
        auto result = get(expression->expr);
        set(expression, result);
    }
    void postorder(const IR::MethodCallExpression* expression) {
        BUG_CHECK(!lhs, "%1% should not be on the lhs", expression);
        MethodCallDescription mcd(expression, map->refMap, map->typeMap);
        if (mcd.instance->is<BuiltInMethod>()) {
            auto bim = mcd.instance->to<BuiltInMethod>();
            auto base = get(bim->appliedTo);
            cstring name = bim->name.name;
            if (name == IR::Type_Header::setInvalid) {
                if (!reads)
                    // modifies the whole header
                    set(expression, base);
                else
                    set(expression, LocationSet::empty);
                return;
            } else if (name == IR::Type_Header::setValid) {
                if (!reads) {
                    // modifies only the valid field
                    auto v = base->getField(validFieldName);
                    set(expression, v);
                } else {
                    set(expression, LocationSet::empty);
                }
            } else if (name == IR::Type_Stack::push_front ||
                       name == IR::Type_Stack::pop_front) {
                set(expression, base);
                return;
            } else {
                BUG_CHECK(name == IR::Type_Header::isValid,
                          "%1%: unexpected method", bim->name);
                if (reads) {
                    auto v = base->getField(validFieldName);
                    set(expression, v);
                } else {
                    set(expression, LocationSet::empty);
                }
                return;
            }
        }

        // For all other methods we act conservatively:
        // in/inout arguments are read, out/inout arguments are written
        auto result = LocationSet::empty;
        for (auto p : *mcd.substitution.getParameters()) {
            if (reads) {
                if (p->direction == IR::Direction::In || p->direction == IR::Direction::InOut) {
                    auto expr = mcd.substitution.lookup(p);
                    auto val = get(expr);
                    result = result->join(val);
                }
            } else {
                if (p->direction == IR::Direction::Out || p->direction == IR::Direction::InOut) {
                    auto expr = mcd.substitution.lookup(p);
                    auto val = get(expr);
                    result = result->join(val);
                }
            }
        }
        set(expression, result);
    }
 public:
    Locations(const StorageMap* map, bool reads, bool lhs) : map(map), reads(reads), lhs(lhs)
    { CHECK_NULL(map); }
    const LocationSet* getLocations(const IR::Expression* expression) {
        expression->apply(*this);
        return get(expression);
    }
};
}  // namespace

const LocationSet* StorageMap::writes(const IR::Expression* expression, bool lhs) const {
    Locations loc(this, false, lhs);
    return loc.getLocations(expression);
}

const LocationSet* StorageMap::reads(const IR::Expression* expression, bool lhs) const {
    Locations loc(this, true, lhs);
    return loc.getLocations(expression);
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
    // We rely on the fact that the set of keys is the same
    auto result = new Definitions();
    for (auto d : other->definitions) {
        auto loc = d.first;
        auto defs = d.second;
        auto current = ::get(definitions, loc);
        auto merged = current->merge(defs);
        result->definitions.emplace(loc, merged);
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

}  // namespace P4
