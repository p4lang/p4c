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

#include <iostream>

#include <boost/version.hpp>

// TODO: Remove this once we migrate away from Ubuntu 18.
#if BOOST_VERSION >= 106700
#include <boost/container_hash/hash.hpp>
#else
#include <boost/functional/hash.hpp>
#endif

#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/p4/tableApply.h"
#include "ir/dbprint.h"
#include "ir/vector.h"
#include "lib/enumerator.h"
#include "lib/indent.h"
#include "lib/ordered_set.h"
#include "lib/stringify.h"
#include "parserCallGraph.h"

namespace P4 {

// internal name for header valid bit; used only locally
const cstring StorageFactory::validFieldName = "$valid";
const cstring StorageFactory::indexFieldName = "$lastIndex";
const LocationSet *LocationSet::empty = new LocationSet();
ProgramPoint ProgramPoint::beforeStart;

unsigned StorageLocation::crtid = 0;

StorageLocation *StorageFactory::create(const IR::Type *type, cstring name) const {
    if (type->is<IR::Type_Bits>() || type->is<IR::Type_Boolean>() || type->is<IR::Type_Varbits>() ||
        type->is<IR::Type_Enum>() || type->is<IR::Type_SerEnum>() || type->is<IR::Type_Error>() ||
        // Since we don't have any operations except assignment for a
        // type described by a type variable, we treat is as a base type.
        type->is<IR::Type_Var>() ||
        // Also for newtype
        type->is<IR::Type_Newtype>())
        return new BaseLocation(type, name);
    if (auto bl = type->to<IR::Type_BaseList>()) {
        // A tuple with no fields is treated like a base location.
        // The other tuples are treated as a collection of their
        // fields.  We need to treat the empty tuple specially because
        // otherwise, since there are no fields, assignments like t =
        // {} would be treated as doing nothing.  However, these
        // assignments do something: they intialize the value
        // (although it's not clear what an uninitialized value of
        // type empty tuple could be).
        if (bl->getSize() == 0) return new BaseLocation(type, name);

        // Tuple and List
        auto result = new TupleLocation(type, name);
        size_t index = 0;
        for (auto t : bl->components) {
            cstring fieldName = name + "[" + Util::toString(index) + "]";
            auto sl = create(t, fieldName);
            result->createElement(index, sl);
            index++;
        }
        return result;
    }
    if (auto st = type->to<IR::Type_StructLike>()) {
        if (st->is<IR::Type_Struct>() && st->fields.size() == 0)
            // See the comment above about empty tuples
            return new BaseLocation(type, name);
        auto result = new StructLocation(type, name);

        // For header unions we will model all of the valid fields
        // for all components as a single shared field.  The
        // reason is that updating one of may change all of the
        // other ones.
        StorageLocation *globalValid = nullptr;
        if (type->is<IR::Type_HeaderUnion>())
            globalValid = create(IR::Type_Boolean::get(), name + "." + validFieldName);

        for (auto f : st->fields) {
            cstring fieldName = name + "." + f->name;
            auto sl = create(f->type, fieldName);
            if (globalValid != nullptr)
                dynamic_cast<StructLocation *>(sl)->replaceField(fieldName + "." + validFieldName,
                                                                 globalValid);
            result->createField(f->name.name, sl);
        }
        if (st->is<IR::Type_Header>()) {
            auto valid = create(IR::Type_Boolean::get(), name + "." + validFieldName);
            result->createField(validFieldName, valid);
        }
        return result;
    } else if (auto st = type->to<IR::Type_Stack>()) {
        auto result = new ArrayLocation(st, name);
        for (unsigned i = 0; i < st->getSize(); i++) {
            auto sl = create(st->elementType, name + "[" + Util::toString(i) + "]");
            result->createElement(i, sl);
        }
        result->setLastIndexField(create(IR::Type_Bits::get(32), name + "." + indexFieldName));
        return result;
    }
    return nullptr;
}

const LocationSet *StorageLocation::removeHeaders() const {
    auto result = new LocationSet();
    removeHeaders(result);
    return result;
}

void BaseLocation::removeHeaders(LocationSet *result) const { result->add(this); }

void StructLocation::addValidBits(LocationSet *result) const {
    if (type->is<IR::Type_Header>()) {
        addField(StorageFactory::validFieldName, result);
    } else {
        for (auto f : fields()) f->addValidBits(result);
    }
}

void StructLocation::addLastIndexField(LocationSet *result) const {
    for (auto f : fields()) f->addLastIndexField(result);
}

void StructLocation::addField(cstring field, LocationSet *result) const {
    auto f = ::get(fieldLocations, field);
    CHECK_NULL(f);
    result->add(f);
}

void TupleLocation::removeHeaders(LocationSet *result) const {
    for (auto f : elements) f->removeHeaders(result);
}

void StructLocation::removeHeaders(LocationSet *result) const {
    if (!type->is<IR::Type_Struct>()) return;
    for (auto f : fieldLocations) f.second->removeHeaders(result);
}

void ArrayLocation::addValidBits(LocationSet *result) const {
    for (auto e : *this) e->addValidBits(result);
}

void ArrayLocation::addLastIndexField(LocationSet *result) const {
    result->add(getLastIndexField());
}

void IndexedLocation::addElement(unsigned index, LocationSet *result) const {
    BUG_CHECK(index < elements.size(), "%1%: out of bounds index", index);
    result->add(elements.at(index));
}

const LocationSet *StorageLocation::getValidBits() const {
    auto result = new LocationSet();
    addValidBits(result);
    return result;
}

const LocationSet *StorageLocation::getLastIndexField() const {
    auto result = new LocationSet();
    addLastIndexField(result);
    return result;
}

const LocationSet *LocationSet::join(const LocationSet *other) const {
    CHECK_NULL(other);
    if (this == LocationSet::empty) return other;
    if (other == LocationSet::empty) return this;
    auto result = new LocationSet(locations);
    for (auto e : other->locations) result->add(e);
    return result;
}

const LocationSet *LocationSet::getArrayLastIndex() const {
    auto result = new LocationSet();
    for (auto l : locations) {
        if (l->is<ArrayLocation>()) {
            auto array = l->to<ArrayLocation>();
            result->add(array->getLastIndexField());
        }
    }
    return result;
}

const LocationSet *LocationSet::getField(cstring field) const {
    auto result = new LocationSet();
    for (auto l : locations) {
        if (auto strct = l->to<StructLocation>()) {
            if (field == StorageFactory::validFieldName && strct->isHeaderUnion()) {
                // special handling for union.isValid()
                for (auto f : strct->fields()) {
                    f->to<StructLocation>()->addField(field, result);
                }
            } else {
                strct->addField(field, result);
            }
        } else if (auto array = l->to<ArrayLocation>()) {
            for (auto f : *array) {
                if (field == IR::Type_Stack::next || field == IR::Type_Stack::last) {
                    result->add(f);
                } else {
                    f->to<StructLocation>()->addField(field, result);
                }
            }
        }
    }
    return result;
}

const LocationSet *LocationSet::getValidField() const {
    return getField(StorageFactory::validFieldName);
}

const LocationSet *LocationSet::getIndex(unsigned index) const {
    auto result = new LocationSet();
    for (auto l : locations) {
        auto array = l->to<IndexedLocation>();
        array->addElement(index, result);
    }
    return result;
}

const LocationSet *LocationSet::allElements() const {
    auto result = new LocationSet();
    for (auto l : locations) {
        auto array = l->to<ArrayLocation>();
        for (auto e : *array) result->add(e);
    }
    return result;
}

const LocationSet *LocationSet::canonicalize() const {
    LocationSet *result = new LocationSet();
    for (auto e : locations) result->addCanonical(e);
    return result;
}

void LocationSet::addCanonical(const StorageLocation *location) {
    if (location->is<BaseLocation>()) {
        add(location);
    } else if (auto wfl = location->to<WithFieldsLocation>()) {
        for (auto f : wfl->fields()) addCanonical(f);
    } else if (auto a = location->to<IndexedLocation>()) {
        for (auto e : *a) addCanonical(e);
    } else {
        BUG("unexpected location");
    }
}

bool LocationSet::overlaps(const LocationSet *other) const {
    for (auto s : locations) {
        if (other->locations.find(s) != other->locations.end()) return true;
    }
    return false;
}

const ProgramPoints *ProgramPoints::merge(const ProgramPoints *with) const {
    auto result = new ProgramPoints(points);
    for (auto p : with->points) result->points.emplace(p);
    return result;
}

ProgramPoint::ProgramPoint(const ProgramPoint &context, const IR::Node *node) {
    for (auto e : context.stack) stack.push_back(e);
    stack.push_back(node);
}

bool ProgramPoint::operator==(const ProgramPoint &other) const {
    if (stack.size() != other.stack.size()) return false;
    for (unsigned i = 0; i < stack.size(); i++)
        if (stack.at(i) != other.stack.at(i)) return false;
    return true;
}

std::size_t ProgramPoint::hash() const {
    std::size_t result = 0;
    boost::hash_range(result, stack.begin(), stack.end());
    return result;
}

bool ProgramPoints::operator==(const ProgramPoints &other) const {
    if (points.size() != other.points.size()) return false;
    for (auto p : points)
        if (other.points.find(p) == other.points.end()) return false;
    return true;
}

Definitions *Definitions::joinDefinitions(const Definitions *other) const {
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
        if (current == nullptr) result->definitions.emplace(loc, defs);
        // otherwise have have already done it in the loop above
    }
    if (unreachable && other->unreachable) result->setUnreachable();
    return result;
}

void Definitions::setDefinition(const StorageLocation *location, const ProgramPoints *point) {
    LocationSet locset;
    locset.addCanonical(location);
    for (auto sl : locset) definitions[sl->to<BaseLocation>()] = point;
}

void Definitions::setDefinition(const LocationSet *locations, const ProgramPoints *point) {
    for (auto sl : *locations->canonicalize()) definitions[sl->to<BaseLocation>()] = point;
}

void Definitions::removeLocation(const StorageLocation *location) {
    auto loc = new LocationSet();
    loc->addCanonical(location);
    for (auto sl : *loc) {
        auto bl = sl->to<BaseLocation>();
        auto it = definitions.find(bl);
        if (it != definitions.end()) definitions.erase(it);
    }
}

const ProgramPoints *Definitions::getPoints(const LocationSet *locations) const {
    const ProgramPoints *result = new ProgramPoints();
    for (auto sl : *locations->canonicalize()) {
        auto points = getPoints(sl->to<BaseLocation>());
        result = result->merge(points);
    }
    return result;
}

Definitions *Definitions::writes(ProgramPoint point, const LocationSet *locations) const {
    auto result = new Definitions(*this);
    auto points = new ProgramPoints();
    points->add(point);
    auto canon = locations->canonicalize();
    for (auto l : *canon) result->setDefinition(l->to<BaseLocation>(), points);
    return result;
}

bool Definitions::operator==(const Definitions &other) const {
    if (definitions.size() != other.definitions.size()) return false;
    for (auto d : definitions) {
        auto od = ::get(other.definitions, d.first);
        if (od == nullptr) return false;
        if (!d.second->operator==(*od)) return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// ComputeWriteSet implementation

// This assumes that all variable declarations have been pushed to the top.
// We could remove this constraint if we also scanned variable declaration initializers.
void ComputeWriteSet::enterScope(const IR::ParameterList *parameters,
                                 const IR::IndexedVector<IR::Declaration> *locals,
                                 ProgramPoint entryPoint, bool clear) {
    Definitions *defs = nullptr;
    if (!clear) defs = currentDefinitions;
    if (defs == nullptr) defs = new Definitions();

    auto startPoints = new ProgramPoints(entryPoint);
    auto uninit = new ProgramPoints(ProgramPoint::beforeStart);

    if (parameters != nullptr) {
        for (auto p : parameters->parameters) {
            StorageLocation *loc = allDefinitions->storageMap->getOrAdd(p);
            if (loc == nullptr) continue;
            if (p->direction == IR::Direction::In || p->direction == IR::Direction::InOut ||
                p->direction == IR::Direction::None)
                defs->setDefinition(loc, startPoints);
            else if (p->direction == IR::Direction::Out)
                defs->setDefinition(loc, uninit);
            auto valid = loc->getValidBits();
            defs->setDefinition(valid, startPoints);
            auto lastIndex = loc->getLastIndexField();
            defs->setDefinition(lastIndex, startPoints);
        }
    }
    if (locals != nullptr) {
        for (auto d : *locals) {
            if (d->is<IR::Declaration_Variable>()) {
                StorageLocation *loc = allDefinitions->storageMap->getOrAdd(d);
                if (loc != nullptr) {
                    defs->setDefinition(loc, uninit);
                    auto valid = loc->getValidBits();
                    defs->setDefinition(valid, startPoints);
                    auto lastIndex = loc->getLastIndexField();
                    defs->setDefinition(lastIndex, startPoints);
                }
            }
        }
    }
    allDefinitions->setDefinitionsAt(entryPoint, defs, false);
    currentDefinitions = defs;
    LOG3("CWS Entered scope " << entryPoint << " definitions are " << Log::endl << defs);
}

void ComputeWriteSet::exitScope(const IR::ParameterList *parameters,
                                const IR::IndexedVector<IR::Declaration> *locals) {
    currentDefinitions = currentDefinitions->cloneDefinitions();
    if (parameters != nullptr) {
        for (auto p : parameters->parameters) {
            StorageLocation *loc = allDefinitions->storageMap->getStorage(p);
            if (loc != nullptr) currentDefinitions->removeLocation(loc);
        }
    }
    if (locals != nullptr) {
        for (auto d : *locals) {
            if (d->is<IR::Declaration_Variable>()) {
                StorageLocation *loc = allDefinitions->storageMap->getStorage(d);
                if (loc != nullptr) currentDefinitions->removeLocation(loc);
            }
        }
    }
}

Definitions *ComputeWriteSet::getDefinitionsAfter(const IR::ParserState *state) {
    ProgramPoint last;
    if (state->components.size() == 0)
        last = ProgramPoint(state);
    else
        last = ProgramPoint(ProgramPoint(state), state->components.back());
    return allDefinitions->getDefinitions(last);
}

// if node is nullptr, use getOriginal().
ProgramPoint ComputeWriteSet::getProgramPoint(const IR::Node *node) const {
    if (node == nullptr) {
        node = getOriginal<IR::Statement>();
        CHECK_NULL(node);
    }
    return ProgramPoint(callingContext, node);
}

// set the currentDefinitions after executing node
bool ComputeWriteSet::setDefinitions(Definitions *defs, const IR::Node *node, bool overwrite) {
    CHECK_NULL(defs);
    currentDefinitions = defs;
    auto point = getProgramPoint(node);
    // Since parsers allow revisiting states multiple times, we allow
    // overwriting always in parser states.  In this case we actually expect
    // that the definitions are monotonically increasing.
    if (findContext<IR::ParserState>()) overwrite = true;
    allDefinitions->setDefinitionsAt(point, currentDefinitions, overwrite);
    LOG3("CWS Definitions at " << point << " are " << Log::endl << defs);
    return false;  // always returns false
}

////////////////////////////
//// visitor implementation

/// For expressions we maintain the write-set in the writes std::map

bool ComputeWriteSet::preorder(const IR::Expression *expression) {
    expressionWrites(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::DefaultExpression *expression) {
    expressionWrites(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::InvalidHeader *expression) {
    expressionWrites(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::InvalidHeaderUnion *expression) {
    expressionWrites(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::StructExpression *expression) {
    expressionWrites(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::HeaderStackExpression *expression) {
    expressionWrites(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::P4ListExpression *expression) {
    expressionWrites(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::Literal *expression) {
    expressionWrites(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::Slice *expression) {
    visit(expression->e0);
    auto base = getWrites(expression->e0);
    if (lhs)
        expressionWrites(expression, base);
    else
        expressionWrites(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::TypeNameExpression *expression) {
    expressionWrites(expression, LocationSet::empty);
    return false;
}

bool ComputeWriteSet::preorder(const IR::PathExpression *expression) {
    if (!lhs) {
        expressionWrites(expression, LocationSet::empty);
        return false;
    }
    auto decl = storageMap->refMap->getDeclaration(expression->path, true);
    auto storage = storageMap->getStorage(decl);
    const LocationSet *result;
    if (storage != nullptr)
        result = new LocationSet(storage);
    else
        result = LocationSet::empty;
    expressionWrites(expression, result);
    return false;
}

bool ComputeWriteSet::preorder(const IR::Member *expression) {
    visit(expression->expr);
    if (!lhs) {
        expressionWrites(expression, LocationSet::empty);
        return false;
    }
    auto type = storageMap->typeMap->getType(expression, true);
    if (type->is<IR::Type_Method>()) return false;
    if (TableApplySolver::isHit(expression, storageMap->refMap, storageMap->typeMap) ||
        TableApplySolver::isMiss(expression, storageMap->refMap, storageMap->typeMap) ||
        TableApplySolver::isActionRun(expression, storageMap->refMap, storageMap->typeMap))
        return false;
    auto storage = getWrites(expression->expr);

    auto basetype = storageMap->typeMap->getType(expression->expr, true);
    if (basetype->is<IR::Type_Stack>()) {
        if (expression->member.name == IR::Type_Stack::next ||
            expression->member.name == IR::Type_Stack::last) {
            expressionWrites(expression, storage);
            return false;
        } else if (expression->member.name == IR::Type_Stack::lastIndex) {
            expressionWrites(expression, LocationSet::empty);
            return false;
        }
    }

    auto fields = storage->getField(expression->member);
    expressionWrites(expression, fields);
    return false;
}

bool ComputeWriteSet::preorder(const IR::ArrayIndex *expression) {
    visit(expression->left);
    auto save = lhs;
    lhs = false;
    visit(expression->right);
    lhs = save;
    auto storage = getWrites(expression->left);
    if (expression->right->is<IR::Constant>()) {
        if (!lhs) {
            expressionWrites(expression, LocationSet::empty);
        } else {
            auto cst = expression->right->to<IR::Constant>();
            auto index = cst->asInt();
            auto result = storage->getIndex(index);
            expressionWrites(expression, result);
        }
    } else {
        expressionWrites(expression, storage->allElements());
    }
    return false;
}

bool ComputeWriteSet::preorder(const IR::Operation_Binary *expression) {
    visit(expression->left);
    visit(expression->right);
    auto l = getWrites(expression->left);
    auto r = getWrites(expression->right);
    auto result = l->join(r);
    expressionWrites(expression, result);
    return false;
}

bool ComputeWriteSet::preorder(const IR::Mux *expression) {
    visit(expression->e0);
    visit(expression->e1);
    visit(expression->e2);
    auto e0 = getWrites(expression->e0);
    auto e1 = getWrites(expression->e1);
    auto e2 = getWrites(expression->e2);
    auto result = e0->join(e1)->join(e2);
    expressionWrites(expression, result);
    return false;
}

bool ComputeWriteSet::preorder(const IR::SelectExpression *expression) {
    BUG_CHECK(!lhs, "%1%: unexpected in lhs", expression);
    visit(expression->select);
    visit(&expression->selectCases);
    auto l = getWrites(expression->select);
    for (auto c : expression->selectCases) {
        auto s = getWrites(c->keyset);
        l = l->join(s);
    }
    expressionWrites(expression, l);
    return false;
}

bool ComputeWriteSet::preorder(const IR::ListExpression *expression) {
    visit(expression->components, "components");
    auto l = LocationSet::empty;
    for (auto c : expression->components) {
        auto cl = getWrites(c);
        l = l->join(cl);
    }
    expressionWrites(expression, l);
    return false;
}

bool ComputeWriteSet::preorder(const IR::Operation_Unary *expression) {
    visit(expression->expr);
    auto result = getWrites(expression->expr);
    expressionWrites(expression, result);
    return false;
}

bool ComputeWriteSet::preorder(const IR::MethodCallExpression *expression) {
    LOG3("CWS Visiting " << dbp(expression));
    bool save = lhs;
    lhs = true;
    // The method call may modify the object, which is part of the method
    visit(expression->method);
    lhs = save;
    auto mi = MethodInstance::resolve(expression, storageMap->refMap, storageMap->typeMap);
    if (auto bim = mi->to<BuiltInMethod>()) {
        auto base = getWrites(bim->appliedTo);
        cstring name = bim->name.name;
        if (name == IR::Type_Header::setInvalid || name == IR::Type_Header::setValid) {
            // modifies only the valid field.
            // setInvalid may in fact write all fields, but
            // it will not really "define" them.
            auto v = base->getField(StorageFactory::validFieldName);
            expressionWrites(expression, v);
            return false;
        } else if (name == IR::Type_Stack::push_front || name == IR::Type_Stack::pop_front) {
            expressionWrites(expression, base);
            return false;
        } else {
            BUG_CHECK(name == IR::Type_Header::isValid, "%1%: unexpected method", bim->name);
            expressionWrites(expression, LocationSet::empty);
            return false;
        }
    }

    // Symbolically call some apply methods (actions and tables)
    std::vector<const IR::IDeclaration *> callees;
    if (mi->is<ActionCall>()) {
        auto action = mi->to<ActionCall>()->action;
        callees.push_back(action);
    } else if (mi->isApply()) {
        auto am = mi->to<ApplyMethod>();
        if (am->isTableApply()) {
            auto table = am->object->to<IR::P4Table>();
            callees.push_back(table);
        }
    } else if (auto em = mi->to<ExternMethod>()) {
        // symbolically call all the methods that might be called via this extern method
        callees = em->mayCall();
    }
    if (!callees.empty()) {
        Log::TempIndent indent;
        LOG3("Analyzing callees of " << expression << DBPrint::Brief << callees << DBPrint::Reset
                                     << indent);
        ProgramPoint pt(callingContext, expression);
        ComputeWriteSet cw(this, pt, currentDefinitions);
        cw.setCalledBy(this);
        for (auto c : callees) (void)c->getNode()->apply(cw);
        currentDefinitions = cw.currentDefinitions;
        exitDefinitions = exitDefinitions->joinDefinitions(cw.exitDefinitions);
        LOG3("Definitions after call of " << DBPrint::Brief << expression << ":" << Log::endl
                                          << currentDefinitions << DBPrint::Reset);
    }

    auto result = LocationSet::empty;
    // For all methods out/inout arguments are written
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto arg = mi->substitution.lookup(p);
        bool save = lhs;
        // pretend we are on the lhs
        lhs = true;
        visit(arg);
        lhs = save;
        if (p->direction == IR::Direction::Out || p->direction == IR::Direction::InOut) {
            auto val = getWrites(arg->expression);
            result = result->join(val);
        }
    }
    expressionWrites(expression, result);
    return false;
}

//////////////////////////
/// Statements and other control structures

void ComputeWriteSet::visitVirtualMethods(const IR::IndexedVector<IR::Declaration> &locals) {
    for (auto l : locals) {
        if (auto li = l->to<IR::Declaration_Instance>()) {
            if (li->initializer) {
                virtualMethod = true;
                // This is a blockStatement that contains all abstract method implementations
                visit(li->initializer);
                virtualMethod = false;
            }
        }
    }
}

// Symbolic execution of the parser
bool ComputeWriteSet::preorder(const IR::P4Parser *parser) {
    LOG3("CWS Visiting " << dbp(parser));
    auto startState = parser->getDeclByName(IR::ParserState::start)->to<IR::ParserState>();
    auto startPoint = ProgramPoint(startState);
    enterScope(parser->getApplyParameters(), &parser->parserLocals, startPoint);
    visitVirtualMethods(parser->parserLocals);

    ParserCallGraph transitions("transitions");
    ComputeParserCG pcg(storageMap->refMap, &transitions);
    pcg.setCalledBy(this);

    (void)parser->apply(pcg);
    ordered_set<const IR::ParserState *> toRun;  // worklist
    toRun.emplace(startState);

    while (!toRun.empty()) {
        auto state = *toRun.begin();
        toRun.erase(state);
        LOG3("Traversing " << dbp(state));

        // We need a new visitor to visit the state,
        // but we use the same data structures
        ProgramPoint pt(state);
        currentDefinitions = allDefinitions->getDefinitions(pt);
        ComputeWriteSet cws(this, pt, currentDefinitions);
        cws.setCalledBy(this);
        (void)state->apply(cws);

        ProgramPoint sp(state);
        auto after = getDefinitionsAfter(state);
        auto next = transitions.getCallees(state);
        for (auto n : *next) {
            ProgramPoint pt(n);
            auto defs = allDefinitions->getDefinitions(pt, true);
            auto newdefs = defs->joinDefinitions(after);
            if (!(*defs == *newdefs)) {
                // Only run once more if there are any changes
                setDefinitions(newdefs, n, true);
                toRun.emplace(n);
            }
        }
    }
    return false;
}

bool ComputeWriteSet::preorder(const IR::P4Control *control) {
    LOG3("CWS Visiting " << dbp(control));
    auto startPoint = ProgramPoint(control);
    enterScope(control->getApplyParameters(), &control->controlLocals, startPoint);
    exitDefinitions = new Definitions();
    returnedDefinitions = new Definitions();
    visitVirtualMethods(control->controlLocals);
    visit(control->body);
    auto returned = currentDefinitions->joinDefinitions(returnedDefinitions);
    auto exited = returned->joinDefinitions(exitDefinitions);
    return setDefinitions(exited, control->body, true);  // overwrite
}

bool ComputeWriteSet::preorder(const IR::IfStatement *statement) {
    LOG3("CWS Visiting " << dbp(statement));
    if (currentDefinitions->isUnreachable()) return setDefinitions(currentDefinitions);
    visit(statement->condition);
    auto cond = getWrites(statement->condition);
    // defs are the definitions after evaluating the condition
    auto defs = currentDefinitions->writes(getProgramPoint(), cond);
    (void)setDefinitions(defs, statement->condition, false);
    visit(statement->ifTrue);
    auto result = currentDefinitions;
    currentDefinitions = defs;
    if (statement->ifFalse != nullptr) visit(statement->ifFalse);
    result = result->joinDefinitions(currentDefinitions);
    return setDefinitions(result);
}

bool ComputeWriteSet::preorder(const IR::BlockStatement *statement) {
    if (currentDefinitions->isUnreachable()) return setDefinitions(currentDefinitions);
    visit(statement->components, "components");
    return setDefinitions(currentDefinitions);
}

bool ComputeWriteSet::preorder(const IR::ReturnStatement *statement) {
    if (statement->expression != nullptr) visit(statement->expression);
    returnedDefinitions = returnedDefinitions->joinDefinitions(currentDefinitions);
    LOG3("Return definitions " << returnedDefinitions);
    auto defs = currentDefinitions->cloneDefinitions();
    defs->setUnreachable();
    return setDefinitions(defs);
}

bool ComputeWriteSet::preorder(const IR::ExitStatement *) {
    if (currentDefinitions->isUnreachable()) return setDefinitions(currentDefinitions);
    exitDefinitions = exitDefinitions->joinDefinitions(currentDefinitions);
    LOG3("Exit definitions " << exitDefinitions);
    auto defs = currentDefinitions->cloneDefinitions();
    defs->setUnreachable();
    return setDefinitions(defs);
}

bool ComputeWriteSet::preorder(const IR::EmptyStatement *) {
    return setDefinitions(currentDefinitions);
}

bool ComputeWriteSet::preorder(const IR::AssignmentStatement *statement) {
    LOG3("CWS Visiting " << dbp(statement) << " " << statement);
    if (currentDefinitions->isUnreachable()) return setDefinitions(currentDefinitions);
    lhs = true;
    visit(statement->left);
    lhs = false;
    visit(statement->right);
    const LocationSet *locs;
    auto l = getWrites(statement->left);
    auto r = getWrites(statement->right);
    locs = l->join(r);
    auto defs = currentDefinitions->writes(getProgramPoint(), locs);
    return setDefinitions(defs);
}

bool ComputeWriteSet::preorder(const IR::SwitchStatement *statement) {
    LOG3("CWS Visiting " << dbp(statement));
    if (currentDefinitions->isUnreachable()) return setDefinitions(currentDefinitions);
    visit(statement->expression);
    auto locs = getWrites(statement->expression);
    auto defs = currentDefinitions->writes(getProgramPoint(statement->expression), locs);
    (void)setDefinitions(defs, statement->expression, false);
    auto save = currentDefinitions;
    auto result = new Definitions();
    bool seenDefault = false;
    for (auto s : statement->cases) {
        currentDefinitions = save;
        if (s->label->is<IR::DefaultExpression>()) seenDefault = true;
        visit(s->statement);
        result = result->joinDefinitions(currentDefinitions);
    }
    auto table = TableApplySolver::isActionRun(statement->expression, storageMap->refMap,
                                               storageMap->typeMap);
    if (table) {
        auto al = table->getActionList();
        bool allCases = statement->cases.size() == al->size();
        if (!seenDefault && !allCases)
            // no case may have been executed
            result = result->joinDefinitions(save);
    } else {
        // TODO: in some cases we can check for exhaustive matches,
        // but this is conservative.
        result = result->joinDefinitions(save);
    }
    return setDefinitions(result);
}

bool ComputeWriteSet::preorder(const IR::P4Action *action) {
    LOG3("CWS Visiting " << dbp(action));
    auto saveReturned = returnedDefinitions;
    returnedDefinitions = new Definitions();

    auto decls = new IR::IndexedVector<IR::Declaration>();
    // We assume that there are no declarations in inner scopes
    // inside the action body.
    for (auto s : action->body->components) {
        if (s->is<IR::Declaration>()) decls->push_back(s->to<IR::Declaration>());
    }
    ProgramPoint pt(callingContext, action);
    enterScope(action->parameters, decls, pt, false);
    visit(action->body);
    currentDefinitions = currentDefinitions->joinDefinitions(returnedDefinitions);
    setDefinitions(currentDefinitions, action->body, true);  // overwrite
    exitScope(action->parameters, decls);
    returnedDefinitions = saveReturned;
    return false;
}

namespace {
class GetDeclarations : public Inspector {
    IR::IndexedVector<IR::Declaration> *declarations;
    bool preorder(const IR::Declaration_Variable *declaration) override {
        declarations->push_back(declaration);
        return true;
    }
    bool preorder(const IR::Declaration_Instance *declaration) override {
        declarations->push_back(declaration);
        return true;
    }

 public:
    GetDeclarations() : declarations(new IR::IndexedVector<IR::Declaration>()) {
        setName("GetDeclarations");
    }
    static IR::IndexedVector<IR::Declaration> *get(const IR::Node *node, const Visitor *calledBy) {
        GetDeclarations gd;
        gd.setCalledBy(calledBy);
        (void)node->apply(gd);
        return gd.declarations;
    }
};
}  // namespace

bool ComputeWriteSet::preorder(const IR::Function *function) {
    if (virtualMethod) {
        LOG3("Virtual method");
        // We may not know where all virtual methods get called from; when
        // this flag is true we are visiting the method without any context,
        // as if it is a global function.
        callingContext = ProgramPoint::beforeStart;
    }
    LOG3("CWS Visiting " << dbp(function) << " called from " << callingContext);
    auto point = ProgramPoint(callingContext, function);
    auto locals = GetDeclarations::get(function->body, this);
    auto saveReturned = returnedDefinitions;
    enterScope(function->type->parameters, locals, point, false);

    returnedDefinitions = new Definitions();
    visit(function->body);
    currentDefinitions = currentDefinitions->joinDefinitions(returnedDefinitions);
    LOG3("CWS @" << point.after() << "=" << currentDefinitions);
    allDefinitions->setDefinitionsAt(point.after(), currentDefinitions, false);
    exitScope(function->type->parameters, locals);

    returnedDefinitions = saveReturned;
    LOG3("Done " << dbp(function));
    return false;
}

bool ComputeWriteSet::preorder(const IR::P4Table *table) {
    LOG3("CWS Visiting " << dbp(table));
    ProgramPoint pt(callingContext, table);
    enterScope(nullptr, nullptr, pt, false);

    // non-deterministic call of one of the actions in the table
    auto after = new Definitions();
    auto beforeTable = currentDefinitions;
    auto actions = table->getActionList();
    for (auto ale : actions->actionList) {
        BUG_CHECK(ale->expression->is<IR::MethodCallExpression>(),
                  "%1%: unexpected entry in action list", ale);
        currentDefinitions = beforeTable;
        visit(ale->expression);
        after = after->joinDefinitions(currentDefinitions);
    }
    exitScope(nullptr, nullptr);
    currentDefinitions = after;
    return false;
}

bool ComputeWriteSet::preorder(const IR::MethodCallStatement *statement) {
    if (currentDefinitions->isUnreachable()) return setDefinitions(currentDefinitions);
    lhs = false;
    visit(statement->methodCall);
    auto locs = getWrites(statement->methodCall);
    auto defs = currentDefinitions->writes(getProgramPoint(), locs);
    return setDefinitions(defs, statement, true);  // overwrite
}

}  // namespace P4

// functions for calling from gdb
void dump(const P4::StorageLocation *s) { std::cout << *s << Log::endl; }
void dump(const P4::StorageMap *s) { std::cout << *s << Log::endl; }
void dump(const P4::LocationSet *s) { std::cout << *s << Log::endl; }
void dump(const P4::ProgramPoint *p) { std::cout << *p << Log::endl; }
void dump(const P4::ProgramPoint &p) { std::cout << p << Log::endl; }
void dump(const P4::ProgramPoints *p) { std::cout << *p << Log::endl; }
void dump(const P4::ProgramPoints &p) { std::cout << p << Log::endl; }
void dump(const P4::Definitions *d) { std::cout << *d << Log::endl; }
void dump(const P4::AllDefinitions *d) { std::cout << *d << Log::endl; }
