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

#include "simplifyDefUse.h"
#include "frontends/p4/def_use.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/tableApply.h"
#include "frontends/p4/ternaryBool.h"
#include "frontends/p4/sideEffects.h"
#include "frontends/p4/parserCallGraph.h"

namespace P4 {

namespace {

class HasUses {
    // Set of program points whose left-hand sides are used elsewhere
    // in the program together with their use count
    std::set<const IR::Node*> used;

    class SliceTracker {
        const IR::Slice* trackedSlice = nullptr;
        bool active = false;
        bool overwritesPrevious(const IR::Slice *previous) {
            if (trackedSlice->getH() >= previous->getH() &&
                trackedSlice->getL() <= previous->getL())
                // current overwrites the previous
                return true;

            return false;
        }

     public:
        SliceTracker() = default;
        explicit SliceTracker(const IR::Slice* slice) :
            trackedSlice(slice), active(true) { }
        bool isActive() const { return active; }

        // main logic of this class
        bool overwrites(const ProgramPoint previous) {
            if (!isActive()) return false;
            if (previous.isBeforeStart()) return false;
            auto last = previous.last();
            if (auto *assign_stmt = last->to<IR::AssignmentStatement>()) {
                if (auto *slice_stmt = assign_stmt->left->to<IR::Slice>()) {
                    // two slice stmts writing to same location
                    // skip use of previous if it gets overwritten
                    if (overwritesPrevious(slice_stmt)) {
                        LOG4("Skipping " << dbp(last) << " " << last);
                        return true;
                    }
                }
            }

            return false;
        }
    };

    SliceTracker tracker;

 public:
    HasUses() = default;
    void add(const ProgramPoints* points) {
        for (auto e : *points) {
            // skips overwritten slice statements
            if (tracker.overwrites(e)) continue;

            auto last = e.last();
            if (last != nullptr) {
                LOG3("Found use for " << dbp(last) << " " <<
                     (last->is<IR::Statement>() ? last : nullptr));
                used.emplace(last);
            }
        }
    }
    bool hasUses(const IR::Node* node) const
    { return used.find(node) != used.end(); }

    void watchForOverwrites(const IR::Slice* slice) {
        BUG_CHECK(!tracker.isActive(), "Call to SliceTracker, but it's already active");
        tracker = SliceTracker(slice);
    }

    void doneWatching() {
        tracker = SliceTracker();
    }
};

/// Creates temporary expressions from declarations and updates the refMap and typeMap
class DeclarationToExpression {
    static DeclarationToExpression* instance;

    /// Stores the temporary expressions so they can be reused
    ordered_map<const IR::Declaration*, const IR::PathExpression*> paths;

 public:
    static DeclarationToExpression* getInstance() {
        if (!instance)
            instance = new DeclarationToExpression;
        return instance;
    }

    const IR::PathExpression* getExpression(const IR::Declaration* decl, ReferenceMap* refMap,
                                            TypeMap* typeMap) {
        CHECK_NULL(decl);
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);

        auto expr = ::get(paths, decl);
        if (!expr) {
            expr = new IR::PathExpression(decl->name);
        }
        if (!refMap->getDeclaration(expr->path, false)) {
            refMap->setDeclaration(expr->path, decl);
            typeMap->setType(expr, typeMap->getType(decl, true));
            paths[decl] = expr;
        }
        return expr;
    }
};

DeclarationToExpression* DeclarationToExpression::instance = nullptr;

class HeaderDefinitions {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    StorageMap* storageMap;

    /// The current values of the header valid bits are stored here. If the value in the map is Yes,
    /// then the header is currently valid. If the value in the map is No, then the header is
    /// currently invalid. If the value in the map is Maybe, then the header is potentially invalid
    /// (for example, this can happen when the header is valid at the end of the then branch and
    /// invalid at the end of the else branch of an if statement, or if the header is valid entering
    /// a parser state on some input branches and invalid on some other)
    ordered_map<const StorageLocation*, TernaryBool> defs;

    /// Currently isValid() expressions in if conditions are not processed, so all headers
    /// for which isValid() is called are temporarly stored here until the end of the block
    /// or until the valid bit is changed again in the block.
    ordered_set<const StorageLocation*> notReport;

 public:
    HeaderDefinitions(ReferenceMap* refMap, TypeMap* typeMap, StorageMap* storageMap) :
        refMap(refMap), typeMap(typeMap), storageMap(storageMap)
        { CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(storageMap); }

    /// A helper function for getting a storage location from an expression.
    /// In case of accessing a header stack with non-constant index, it returns
    /// storage locations of all elements within the stack. In case of accessing
    /// a field of a header union within a stack (indexed with non-constant), it
    /// returns the corresponding field of all unions in the stack.
    const LocationSet* getStorageLocation(const IR::Expression* expression) const {
        LocationSet* result = new LocationSet;
        if (auto expr = expression->to<IR::PathExpression>()) {
            auto decl = refMap->getDeclaration(expr->path, true);
            result->add(storageMap->getStorage(decl));
        } else if (auto expr = expression->to<IR::Member>()) {
            auto base_storage = getStorageLocation(expr->expr);
            for (auto bs : *base_storage) {
                if (auto struct_storage = bs->to<StructLocation>()) {
                    struct_storage->addField(expr->member, result);
                } else if (bs->is<ArrayLocation>() &&
                          (expr->member == IR::Type_Stack::next ||
                           expr->member == IR::Type_Stack::last ||
                           expr->member == IR::Type_Stack::lastIndex)) {
                    auto array_storage = bs->to<ArrayLocation>();
                    for (auto element : *array_storage)
                        result->add(element);
                }
            }
        } else if (auto array = expression->to<IR::ArrayIndex>()) {
            auto base_storage = getStorageLocation(array->left);
            for (auto bs : *base_storage) {
                if (auto array_storage = bs->to<ArrayLocation>()) {
                    if (auto index = array->right->to<IR::Constant>()) {
                        array_storage->addElement(index->asInt(), result);
                    } else {
                        for (auto element : *array_storage)
                            result->add(element);
                    }
                }
            }
        }
        return result;
    }

    /// In case of a header, it simply sets its value in the map. In case
    /// of header unions and stacks, it sets the value to all their fields.
    void setValueToStorage(const StorageLocation* storage, TernaryBool value) {
        if (!storage)
            return;

        if (auto struct_storage = storage->to<StructLocation>()) {
            if (struct_storage->isHeader()) {
                update(struct_storage, value);
            } else {
                for (auto f : struct_storage->fields())
                    setValueToStorage(f, value);

                // update the valid bit of a union itself
                if (struct_storage->isHeaderUnion())
                    update(struct_storage, value);
            }
        } else if (auto array_storage = storage->to<ArrayLocation>()) {
            for (auto element : *array_storage)
                setValueToStorage(element, value);
        }
    }

    void checkLocation(const StorageLocation* storage) {
        BUG_CHECK(storage->is<StructLocation>() &&
                        (storage->to<StructLocation>()->isHeader() ||
                         storage->to<StructLocation>()->isHeaderUnion()),
                "location %1% is not a header", storage->name);
    }

    bool isNonConstIndexing(const IR::Expression* expr) const {
        if (auto array = expr->to<IR::ArrayIndex>())
            if (!array->right->to<IR::Constant>())
                return true;

        auto member = expr->to<IR::Member>();
        auto base = member ? member->expr : nullptr;

        if (member && base && typeMap->getType(base, true)->is<IR::Type_Stack>() &&
           (member->member == IR::Type_Stack::next ||
            member->member == IR::Type_Stack::last ||
            member->member == IR::Type_Stack::lastIndex)) {
            return true;
        }

        return false;
    }

    void update(const StorageLocation* storage, TernaryBool valid) {
        CHECK_NULL(storage);
        checkLocation(storage);
        defs[storage] = valid;
        notReport.erase(storage);
    }

    void update(const LocationSet* locations, TernaryBool valid) {
        CHECK_NULL(locations);
        for (auto storage : *locations) {
            update(storage, valid);
        }
    }

    void update(const IR::Expression* expr, TernaryBool valid) {
        CHECK_NULL(expr);

        // skipping invalidation of the whole stack
        if (isNonConstIndexing(expr) && valid != TernaryBool::Yes)
            return;

        auto member = expr->to<IR::Member>();
        if (member && member->expr &&
            typeMap->getType(member->expr, true)->is<IR::Type_HeaderUnion>()) {
            // accessing a field of a union of a stack of unions
            // (with non-constant indexing)
            if (isNonConstIndexing(member->expr)) {
                // skipping invalidation of the whole stack (valid isn't TernaryBool::Yes)
                if (valid == TernaryBool::Yes) {
                    // we don't invalidate other fields (if any is valid) in order to
                    // avoid false positives
                    update(getStorageLocation(expr), valid);
                }
            } else {  // constant index or accessing a field of a union which isn't in a stack
                // invalidate fields of a union
                auto base_storage = getStorageLocation(member->expr);
                for (auto bs : *base_storage) {
                    setValueToStorage(bs, TernaryBool::No);
                }
                // update valid bits of a field and a union
                update(getStorageLocation(expr), valid);
                update(base_storage, valid);
            }
            return;
        }

        update(getStorageLocation(expr), valid);
    }

    TernaryBool find(const StorageLocation* storage) const {
        CHECK_NULL(storage);

        if (notReport.find(storage) != notReport.end())
            return TernaryBool::Yes;

        return ::get(defs, storage, TernaryBool::Maybe);
    }

    // result is OR operation on valid bits of all locations
    TernaryBool find(const LocationSet* locations) const {
        CHECK_NULL(locations);

        if (locations->isEmpty())
            return TernaryBool::Yes;

        TernaryBool valid = TernaryBool::No;
        for (auto storage : *locations) {
            TernaryBool val = find(storage);
            if (val == TernaryBool::Maybe)
                valid = val;
            else if (val == TernaryBool::Yes)
                return TernaryBool::Yes;
        }

        return valid;
    }

    TernaryBool find(const IR::Expression* expr) const {
        CHECK_NULL(expr);
        return find(getStorageLocation(expr));
    }

    void clear() { defs.clear(); }

    HeaderDefinitions* clone() const { return new HeaderDefinitions(*this); }

    bool operator==(const HeaderDefinitions& other) const {
        return defs == other.defs && notReport == other.notReport;
    }

    bool operator!=(const HeaderDefinitions& other) const { return !(*this == other); }

    HeaderDefinitions* intersect(const HeaderDefinitions* other) const {
        HeaderDefinitions* result = new HeaderDefinitions(refMap, typeMap, storageMap);
        for (auto def : defs) {
            auto valid = ::get(other->defs, def.first, TernaryBool::Maybe);
            result->defs.emplace(def.first, valid == def.second ? valid : TernaryBool::Maybe);
        }
        return result;
    }

    void addToNotReport(const LocationSet* locations) {
        CHECK_NULL(locations);
        for (auto storage : *locations) {
            checkLocation(storage);
            notReport.emplace(storage);
            if (auto header_union = storage->to<StructLocation>())
                if (header_union->isHeaderUnion())
                    for (auto field : header_union->fields())
                        notReport.emplace(field);
        }
    }

    void addToNotReport(const IR::Expression* expr) {
        CHECK_NULL(expr);
        addToNotReport(getStorageLocation(expr));
    }

    void setNotReport(const HeaderDefinitions* other) {
        notReport = other->notReport;
    }
};

// Run for each parser and control separately
// Somewhat of a misnamed pass -- the main point of this pass is to find all the uses
// of each definition, and fill in the `hasUses` output with all the definitions that have
// uses so RemoveUnused can remove unused things.  It incidentally finds uses that have
// no definitions and issues uninitialized warnings about them.
class FindUninitialized : public Inspector {
    ProgramPoint    context;    // context as of the last call or state transition
    ReferenceMap*   refMap;
    TypeMap*        typeMap;
    AllDefinitions* definitions;
    bool            lhs;  // checking the lhs of an assignment
    ProgramPoint    currentPoint;  // context of the current expression/statement
    /// For some simple expresssions keep here the read location sets.
    /// This does not include location sets read by subexpressions.
    std::map<const IR::Expression*, const LocationSet*> readLocations;
    HasUses*        hasUses;  // output
    /// If true the current statement is unreachable
    bool            unreachable;
    bool            virtualMethod;

    HeaderDefinitions* headerDefs;
    bool reportInvalidHeaders = true;

    const LocationSet* getReads(const IR::Expression* expression, bool nonNull = false) const {
        auto result = ::get(readLocations, expression);
        if (nonNull)
            BUG_CHECK(result != nullptr, "no locations known for %1%", dbp(expression));
        return result;
    }
    /// 'expression' is reading the 'loc' location set
    void reads(const IR::Expression* expression, const LocationSet* loc) {
        BUG_CHECK(!unreachable, "reached an unreachable expression in FindUninitialized");
        LOG3(expression << " reads " << loc);
        CHECK_NULL(expression);
        CHECK_NULL(loc);
        readLocations.erase(expression);
        readLocations.emplace(expression, loc);
    }
    bool setCurrent(const IR::Statement* statement) {
        currentPoint = ProgramPoint(context, statement);
        LOG3(IndentCtl::unindent);
        return false;
    }
    profile_t init_apply(const IR::Node *root) override {
        unreachable = false;  // assume not unreachable at the start of any apply
        return Inspector::init_apply(root);
    }

    FindUninitialized(FindUninitialized* parent, ProgramPoint context) :
            context(context), refMap(parent->definitions->storageMap->refMap),
            typeMap(parent->definitions->storageMap->typeMap),
            definitions(parent->definitions), lhs(false), currentPoint(context),
            hasUses(parent->hasUses), virtualMethod(false), headerDefs(parent->headerDefs),
            reportInvalidHeaders(parent->reportInvalidHeaders)    { visitDagOnce = false; }

 public:
    FindUninitialized(AllDefinitions* definitions, HasUses* hasUses) :
            refMap(definitions->storageMap->refMap),
            typeMap(definitions->storageMap->typeMap),
            definitions(definitions), lhs(false), currentPoint(),
            hasUses(hasUses), virtualMethod(false),
            headerDefs(new HeaderDefinitions(refMap, typeMap, definitions->storageMap)) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(definitions);
        CHECK_NULL(hasUses);
        visitDagOnce = false; }

    // we control the traversal order manually, so we always 'prune()'
    // (return false from preorder)

    bool preorder(const IR::ParserState* state) override {
        LOG3("FU Visiting state " << state->name);
        context = ProgramPoint(state);
        currentPoint = ProgramPoint(state);  // point before the first statement
        visit(state->components, "components");
        if (state->selectExpression != nullptr)
            visit(state->selectExpression);
        context = ProgramPoint();
        return false;
    }

    Definitions* getCurrentDefinitions() const {
        auto defs = definitions->getDefinitions(currentPoint, true);
        LOG3("FU Current point is (after) " << currentPoint <<
                " definitions are " << IndentCtl::endl << defs);
        return defs;
    }

    // Called at the beginning of controls, parsers and functions
    void initHeaderParams(const IR::ParameterList* parameters) {
        if (!parameters)
            return;
        for (auto p : parameters->parameters)
            if (auto storage = definitions->storageMap->getStorage(p)) {
                headerDefs->setValueToStorage(storage, p->direction != IR::Direction::Out
                                                       ? TernaryBool::Yes
                                                       : TernaryBool::No);
            }
    }

    void checkOutParameters(const IR::IDeclaration* block,
                            const IR::ParameterList* parameters,
                            Definitions* defs) {
        LOG2("Checking output parameters of " << block <<
             "; definitions are " << IndentCtl::endl << defs);
        for (auto p : parameters->parameters) {
            if (p->direction == IR::Direction::Out || p->direction == IR::Direction::InOut) {
                auto storage = definitions->storageMap->getStorage(p);
                LOG3("Checking parameter: " << p);
                if (storage == nullptr)
                    continue;

                const LocationSet* loc = new LocationSet(storage);
                auto points = defs->getPoints(loc);
                hasUses->add(points);
                if (typeMap->typeIsEmpty(storage->type))
                    continue;
                // Check uninitialized non-headers (headers can be invalid).
                // inout parameters can never match here, so we could skip them.
                loc = storage->removeHeaders();
                points = defs->getPoints(loc);
                if (points->containsBeforeStart())
                    warn(ErrorType::WARN_UNINITIALIZED_OUT_PARAM,
                         "out parameter '%1%' may be uninitialized when "
                         "'%2%' terminates", p, block->getName());
            }
        }
    }

    bool preorder(const IR::P4Control* control) override {
        LOG3("FU Visiting control " << control->name << "[" << control->id << "]");
        BUG_CHECK(context.isBeforeStart(), "non-empty context in FindUnitialized::P4Control");
        currentPoint = ProgramPoint(control);
        headerDefs->clear();
        initHeaderParams(control->getApplyMethodType()->parameters);
        visitVirtualMethods(control->controlLocals);
        unreachable = false;
        visit(control->body);
        checkOutParameters(
            control, control->getApplyMethodType()->parameters, getCurrentDefinitions());
        LOG3("FU Returning from " << control->name << "[" << control->id << "]");
        return false;
    }

    bool preorder(const IR::Function* func) override {
        HeaderDefinitions* saveHeaderDefs = nullptr;
        if (virtualMethod) {
            LOG3("Virtual method");
            context = ProgramPoint::beforeStart;
            unreachable = false;
            // we must save the definitions from the outer block
            saveHeaderDefs = headerDefs->clone();
        }
        LOG3("FU Visiting function " << dbp(func) << " called by " << context);
        LOG5(func);
        auto point = ProgramPoint(context, func);
        currentPoint = point;
        initHeaderParams(func->type->parameters);
        visit(func->body);
        bool checkReturn = !func->type->returnType->is<IR::Type_Void>();
        if (checkReturn) {
            auto defs = getCurrentDefinitions();
            // The definitions after the body of the function should
            // contain "unreachable", otherwise it means that we have
            // not executed a 'return' on all possible paths.
            if (!defs->isUnreachable())
                ::error(ErrorType::ERR_INSUFFICIENT,
                        "Function '%1%' does not return a value on all paths", func);
        }

        currentPoint = point.after();
        // We now check the out parameters using the definitions
        // produced *after* the function has completed.
        LOG3("Context after function " << currentPoint);
        auto current = getCurrentDefinitions();
        checkOutParameters(func, func->type->parameters, current);
        if (saveHeaderDefs) {
            headerDefs = saveHeaderDefs;
        }
        return false;
    }

    void visitVirtualMethods(const IR::IndexedVector<IR::Declaration> &locals) {
        // We don't really know when virtual methods may be called, so
        // we visit them proactively once as if they are top-level functions.
        // During this visit the 'virtualMethod' flag is 'true'.
        // We may visit them also when they are invoked by a callee, but
        // at that time the 'virtualMethod' flag will be false.
        auto saveContext = context;
        for (auto l : locals) {
            if (auto li = l->to<IR::Declaration_Instance>()) {
                if (li->initializer) {
                    virtualMethod = true;
                    visit(li->initializer);
                    virtualMethod = false;
                }}}
        context = saveContext;
    }

    bool preorder(const IR::P4Parser* parser) override {
        LOG3("FU Visiting parser " << parser->name << "[" << parser->id << "]");
        currentPoint = ProgramPoint(parser);
        headerDefs->clear();
        initHeaderParams(parser->getApplyMethodType()->parameters);
        visitVirtualMethods(parser->parserLocals);

        auto startState = parser->getDeclByName(IR::ParserState::start)->to<IR::ParserState>();
        auto acceptState = parser->getDeclByName(IR::ParserState::accept)->to<IR::ParserState>();

        ParserCallGraph transitions("transitions");
        ComputeParserCG pcg(refMap, &transitions);
        pcg.setCalledBy(this);

        (void)parser->apply(pcg);
        ordered_set<const IR::ParserState*> toRun;  // worklist
        ordered_map<const IR::ParserState*, HeaderDefinitions*> inputHeaderDefs;

        toRun.emplace(startState);
        inputHeaderDefs.emplace(startState, headerDefs);

        // We do not report warnings until we have all definitions for every parser state
        reportInvalidHeaders = false;

        while (!toRun.empty()) {
            auto state = *toRun.begin();
            toRun.erase(state);
            LOG3("Traversing " << dbp(state));

            // We need a new visitor to visit the state,
            // but we use the same data structures
            headerDefs = inputHeaderDefs[state]->clone();
            FindUninitialized fu(this, currentPoint);
            fu.setCalledBy(this);
            (void)state->apply(fu);

            auto next = transitions.getCallees(state);
            for (auto n : *next) {
                if (inputHeaderDefs.find(n) == inputHeaderDefs.end()) {
                    inputHeaderDefs[n] = headerDefs->clone();
                    toRun.emplace(n);
                } else {
                    auto newInputDefs = inputHeaderDefs[n]->intersect(headerDefs);
                    if (*newInputDefs != *inputHeaderDefs[n]) {
                        inputHeaderDefs[n] = newInputDefs;
                        toRun.emplace(n);
                    }
                }
            }
        }

        reportInvalidHeaders = true;
        for (auto state : parser->states) {
            if (inputHeaderDefs.find(state) == inputHeaderDefs.end()) {
                inputHeaderDefs.emplace(state,
                    new HeaderDefinitions(refMap, typeMap, definitions->storageMap));
            }
            headerDefs = inputHeaderDefs[state];
            visit(state);
        }

        headerDefs = inputHeaderDefs[acceptState];
        unreachable = false;
        auto accept = ProgramPoint(parser->getDeclByName(IR::ParserState::accept)->getNode());
        auto acceptdefs = definitions->getDefinitions(accept, true);
        auto reject = ProgramPoint(parser->getDeclByName(IR::ParserState::reject)->getNode());
        auto rejectdefs = definitions->getDefinitions(reject, true);

        auto outputDefs = acceptdefs->joinDefinitions(rejectdefs);
        checkOutParameters(parser, parser->getApplyMethodType()->parameters, outputDefs);
        LOG3("FU Returning from " << parser->name << "[" << parser->id << "]");
        return false;
    }

    // expr is an sub-expression that appears in the lhs of an assignment.
    // parent is one of it's parent expressions.
    //
    // When we assign to a header we are also implicitly reading the header's
    // valid flag.
    // Consider this example:
    // header H { ... };
    // H a;
    // a.x = 1;  <<< This has an effect only if a is valid.
    //               So this write actually reads the valid flag of a.
    // The function will recurse the structure of expr until it finds
    // a header and will mark the header valid bit as read.
    // It returns the LocationSet of parent.
    const LocationSet* checkHeaderFieldWrite(
        const IR::Expression* expr, const IR::Expression* parent) {
        const LocationSet* loc;
        if (auto mem = parent->to<IR::Member>()) {
            loc = checkHeaderFieldWrite(expr, mem->expr);
            loc = loc->getField(mem->member);
        } else if (auto ai = parent->to<IR::ArrayIndex>()) {
            loc = checkHeaderFieldWrite(expr, ai->left);
            if (auto cst = ai->right->to<IR::Constant>()) {
                auto index = cst->asInt();
                loc = loc->getIndex(index);
            }
            // else let loc be the whole array
        } else if (auto pe = parent->to<IR::PathExpression>()) {
            auto decl = refMap->getDeclaration(pe->path, true);
            auto storage = definitions->storageMap->getStorage(decl);
            if (storage != nullptr)
                loc = new LocationSet(storage);
            else
                loc = LocationSet::empty;
        } else if (auto slice = parent->to<IR::Slice>()) {
            loc = checkHeaderFieldWrite(expr, slice->e0);
        } else {
            BUG("%1%: unexpected expression on LHS", parent);
        }

        auto type = typeMap->getType(parent, true);
        if (type->is<IR::Type_Header>()) {
            if (expr != parent) {
                // If we are writing to an entire header (expr ==
                // parent) we are actually overwriting the valid bit
                // as well.  So we are not reading it.
                loc = loc->getValidField();
                LOG3("Expression " << expr << " reads valid bit " << loc);
                reads(expr, loc);
                registerUses(expr);
            }
        }
        return loc;
    }

    void processHeadersInAssignment(const IR::Expression* dst, const IR::Expression* src,
                                    const IR::Type* dst_type, const IR::Type* src_type) {
        if (!dst || !src || !dst_type || !src_type)
            return;

        if (dst_type->is<IR::Type_Header>()) {
            if (src->is<IR::StructExpression>() || src->is<IR::MethodCallExpression>()) {
                headerDefs->update(dst, TernaryBool::Yes);
            } else if (src_type->is<IR::Type_Header>()) {
                auto valid = headerDefs->find(src);
                headerDefs->update(dst, valid);
            } else {
                BUG("%1%: unexpected expression on RHS", src);
            }
            return;
        }

        if (auto dst_struct = dst_type->to<IR::Type_Struct>()) {
            if (auto list = src->to<IR::StructExpression>()) {
                auto it = list->components.begin();
                for (auto field : dst_struct->fields) {
                    auto ftype = typeMap->getType(field, true);
                    auto member = new IR::Member(dst, field->name);
                    typeMap->setType(member, ftype);
                    processHeadersInAssignment(member, (*it)->expression, ftype,
                                               typeMap->getType((*it)->expression, true));
                    ++it;
                }
            } else if (src->is<IR::MethodCallExpression>()) {
                auto storage = headerDefs->getStorageLocation(dst);
                for (auto s : *storage) {
                    headerDefs->setValueToStorage(s, TernaryBool::Yes);
                }
            } else if (src_type->to<IR::Type_Struct>()) {
                for (auto field : dst_struct->fields) {
                    auto ftype = typeMap->getType(field, true);
                    auto dst_member = new IR::Member(dst, field->name);
                    auto src_member = new IR::Member(src, field->name);
                    typeMap->setType(dst_member, ftype);
                    typeMap->setType(src_member, ftype);
                    processHeadersInAssignment(dst_member, src_member, ftype, ftype);
                }
            } else {
                BUG("%1%: unexpected expression on RHS", src);
            }
            return;
        }

        if (auto dst_headerunion = dst_type->to<IR::Type_HeaderUnion>()) {
            if (src->is<IR::MethodCallExpression>()) {
                auto storage = headerDefs->getStorageLocation(dst);
                for (auto s : *storage) {
                    headerDefs->setValueToStorage(s, TernaryBool::Yes);
                }
            } else if (src_type->is<IR::Type_HeaderUnion>()) {
                auto member = dst->to<IR::Member>();
                bool non_constant_indexing = member ?
                                             headerDefs->isNonConstIndexing(member->expr) :
                                             false;

                for (auto field : dst_headerunion->fields) {
                    auto ftype = typeMap->getType(field, true);
                    auto dst_member = new IR::Member(dst, field->name);
                    auto src_member = new IR::Member(src, field->name);
                    typeMap->setType(dst_member, ftype);
                    typeMap->setType(src_member, ftype);
                    auto valid = headerDefs->find(src_member);
                    if (!non_constant_indexing || valid == TernaryBool::Yes)
                        headerDefs->update(headerDefs->getStorageLocation(dst_member),
                                           valid);
                }
                auto valid = headerDefs->find(src);
                if (!non_constant_indexing || valid == TernaryBool::Yes)
                    headerDefs->update(headerDefs->getStorageLocation(dst), TernaryBool::Yes);
            } else {
                BUG("%1%: unexpected expression on RHS", src);
            }
            return;
        }

        if (dst_type->is<IR::Type_Stack>()) {
            if (src->is<IR::StructExpression>() || src->is<IR::MethodCallExpression>()) {
                auto locations = headerDefs->getStorageLocation(dst);
                for (auto storage : *locations) {
                    headerDefs->setValueToStorage(storage, TernaryBool::Yes);
                }
            } else if (src_type->is<IR::Type_Stack>()) {
                auto dst_locations = headerDefs->getStorageLocation(dst);
                auto src_locations = headerDefs->getStorageLocation(src);

                if (!dst_locations->isEmpty() && !src_locations->isEmpty()) {
                    auto dst_storage = *dst_locations->begin();
                    auto src_storage = *src_locations->begin();

                    auto dst_array_storage = dst_storage->to<ArrayLocation>();
                    auto src_array_storage = src_storage->to<ArrayLocation>();
                    if (dst_array_storage && src_array_storage) {
                        auto it = src_array_storage->begin();
                        for (auto dst_element : *dst_array_storage) {
                            auto dst_header_union = dst_element->to<StructLocation>();
                            auto src_header_union = (*it)->to<StructLocation>();
                            if (dst_header_union->isHeaderUnion() &&
                                src_header_union->isHeaderUnion()) {
                                auto field_it = src_header_union->fields().begin();
                                for (auto field : dst_header_union->fields()) {
                                    headerDefs->update(field, headerDefs->find(*field_it));
                                    ++field_it;
                                }
                            }
                            headerDefs->update(dst_element, headerDefs->find(*it));
                            ++it;
                        }
                    }
                }
            } else {
                BUG("%1%: unexpected expression on RHS", src);
            }
        }
    }

    bool preorder(const IR::AssignmentStatement* statement) override {
        LOG3("FU Visiting " << dbp(statement) << " " << statement << IndentCtl::indent);
        if (!unreachable) {
            lhs = true;
            visit(statement->left);
            checkHeaderFieldWrite(statement->left, statement->left);
            LOG3("FU Returned from " << statement->left);
            lhs = false;
            visit(statement->right);
            LOG3("FU Returned from " << statement->right);
            processHeadersInAssignment(statement->left,
                                       statement->right,
                                       typeMap->getType(statement->left, true),
                                       typeMap->getType(statement->right, true));
        } else {
            LOG3("Unreachable");
        }
        return setCurrent(statement);
    }

    bool preorder(const IR::ReturnStatement* statement) override {
        LOG3("FU Visiting " << statement);
        if (!unreachable && statement->expression != nullptr)
            visit(statement->expression);
        else
            LOG3("Unreachable");
        unreachable = true;
        return setCurrent(statement);
    }

    bool preorder(const IR::ExitStatement* statement) override {
        LOG3("FU Visiting " << statement);
        unreachable = true;
        LOG3("Unreachable");
        return setCurrent(statement);
    }

    bool preorder(const IR::MethodCallStatement* statement) override {
        LOG3("FU Visiting " << statement);
        if (!unreachable)
            visit(statement->methodCall);
        else
            LOG3("Unreachable");

        return setCurrent(statement);
    }

    bool preorder(const IR::BlockStatement* statement) override {
        LOG3("FU Visiting " << statement);
        if (!unreachable) {
            visit(statement->components, "components");
        } else {
            LOG3("Unreachable");
        }
        return setCurrent(statement);
    }

    bool preorder(const IR::IfStatement* statement) override {
        LOG3("FU Visiting " << statement);
        if (!unreachable) {
            auto saveHeaderDefsBeforeCondition = headerDefs->clone();
            visit(statement->condition);
            auto saveHeaderDefsAfterCondition = headerDefs->clone();
            currentPoint = ProgramPoint(context, statement->condition);
            auto saveCurrent = currentPoint;
            auto saveUnreachable = unreachable;
            visit(statement->ifTrue);
            auto unreachableAfterThen = unreachable;
            unreachable = saveUnreachable;
            if (statement->ifFalse != nullptr) {
                currentPoint = saveCurrent;
                std::swap(headerDefs, saveHeaderDefsAfterCondition);
                visit(statement->ifFalse);
            }
            unreachable = unreachableAfterThen && unreachable;
            headerDefs = headerDefs->intersect(saveHeaderDefsAfterCondition);
            headerDefs->setNotReport(saveHeaderDefsBeforeCondition);
        } else {
            LOG3("Unreachable");
        }
        return setCurrent(statement);
    }

    bool preorder(const IR::SwitchStatement* statement) override {
        LOG3("FU Visiting " << statement);
        if (!unreachable) {
            bool finalUnreachable = true;
            bool hasDefault = false;
            auto saveHeaderDefsBeforeExpr = headerDefs->clone();
            visit(statement->expression);
            auto saveHeaderDefsAfterExpr = headerDefs->clone();
            HeaderDefinitions* finalHeaderDefs = nullptr;
            currentPoint = ProgramPoint(context, statement->expression);
            auto saveCurrent = currentPoint;
            auto saveUnreachable = unreachable;
            for (auto c : statement->cases) {
                if (c->statement != nullptr) {
                    LOG3("Visiting " << c);
                    if (c->label->is<IR::DefaultExpression>())
                        hasDefault = true;
                    currentPoint = saveCurrent;
                    unreachable = saveUnreachable;
                    headerDefs = saveHeaderDefsAfterExpr->clone();
                    visit(c);
                    finalUnreachable = finalUnreachable && unreachable;
                    if (finalHeaderDefs) {
                        finalHeaderDefs = finalHeaderDefs->intersect(headerDefs);
                    } else {
                        finalHeaderDefs = headerDefs;
                    }
                }
            }
            unreachable = finalUnreachable;
            if (finalHeaderDefs) {
                if (hasDefault)
                    headerDefs = finalHeaderDefs;
                else
                    headerDefs = finalHeaderDefs->intersect(saveHeaderDefsAfterExpr);
            }
            headerDefs->setNotReport(saveHeaderDefsBeforeExpr);
        } else {
            LOG3("Unreachable");
        }
        return setCurrent(statement);
    }

    ////////////////// Expressions

    bool preorder(const IR::Literal* expression) override {
        reads(expression, LocationSet::empty);
        return false;
    }

    bool preorder(const IR::TypeNameExpression* expression) override {
        reads(expression, LocationSet::empty);
        return false;
    }

    // Check whether the expression the child of a Member or
    // ArrayIndex.  I.e., for and expression such as a.x within a
    // larger expression a.x.b it returns "false".  This is because
    // the expression is not reading a.x, it is reading just a.x.b.
    // ctx must be the context of the current expression in the
    // visitor.
    bool isFinalRead(const Visitor::Context* ctx, const IR::Expression* expression) {
        if (ctx == nullptr)
            return true;

        // If this expression is a child of a Member of a left
        // child of an ArrayIndex then we don't report it here, only
        // in the parent.
        auto parentexp = ctx->node->to<IR::Expression>();
        if (parentexp != nullptr) {
            if (parentexp->is<IR::Member>())
                return false;
            if (parentexp->is<IR::ArrayIndex>()) {
                // Since we are doing the visit using a custom order,
                // ctx->child_index is not accurate, so we check
                // manually whether this is the left child.
                auto ai = parentexp->to<IR::ArrayIndex>();
                if (ai->left == expression)
                    return false;
            }
        }
        return true;
    }

    // Keeps track of which expression producers have uses in the given expression
    void registerUses(const IR::Expression* expression, bool reportUninitialized = true) {
        LOG3("FU Registering uses for '" << expression << "'");
        if (!isFinalRead(getContext(), expression)) {
            LOG3("Expression '" << expression << "' is not fully read. Returning...");
            return;
        }

        auto currentDefinitions = getCurrentDefinitions();
        if (currentDefinitions->isUnreachable()) {
            LOG3("are not reachable. Returning...");
            return;
        }

        const LocationSet* read = getReads(expression);
        if (read == nullptr || read->isEmpty()) {
            LOG3("No LocationSet for '" << expression << "'. Returning...");
            return;
        }
        LOG3("LocationSet for '" << expression << "' is <<" << read << ">>");

        auto points = currentDefinitions->getPoints(read);

        if (reportUninitialized && !lhs && points->containsBeforeStart()
            && hasUninitializedHeaderUnion(expression, currentDefinitions, read)) {
            // Do not report uninitialized values on the LHS.
            // This could happen if we are writing to an array element
            // with an unknown index.
            auto type = typeMap->getType(expression, true);
            cstring message;
            if (type->is<IR::Type_Base>())
                message = "%1% may be uninitialized";
            else
                message = "%1% may not be completely initialized";
            warn(ErrorType::WARN_UNINITIALIZED_USE, message, expression);
        }

        hasUses->add(points);
    }

    // Checks if header unions and header union stacks are initialized.
    // Unlike other StructLike types header unions are initialized
    // if only one member of the union is initialized.
    // Returns true if header union is uninitialized, or if the type is not,
    // or does not contain a header union.
    bool hasUninitializedHeaderUnion(const IR::Expression* expression,
                                     const P4::Definitions* currentDefinitions,
                                     const LocationSet* read) {
        auto type = typeMap->getType(expression, true);

        if (type->is<IR::Type_HeaderUnion>()) {
            if (isHeaderUnionUninitialized(type, currentDefinitions, read)) {
                return true;
            } else {
                return false;
            }
        } else if (type->is<IR::Type_Stack>()) {
            if (isHeaderUnionStackUninitialized(type, currentDefinitions, read)) {
                return true;
            } else {
                return false;
            }
        } else {
            return true;
        }
    }

    // Checks if a header union is uninitialized
    bool isHeaderUnionUninitialized(const IR::Type* type,
                                    const P4::Definitions* currentDefinitions,
                                    const LocationSet* read) {
        auto huType = type->to<IR::Type_HeaderUnion>();
        for (auto header : huType->fields) {
            auto headerLoc = read->getField(header->name);
            auto points = currentDefinitions->getPoints(headerLoc);
            if (!points->containsBeforeStart()) {
                return false;
            }
        }
        return true;
    }

    // Checks if a header union stack is uninitialized
    bool isHeaderUnionStackUninitialized(const IR::Type* type,
                                         const P4::Definitions* currentDefinitions,
                                         const LocationSet* read) {
        auto sType = type->to<IR::Type_Stack>();
        for (unsigned int i=0; i<sType->getSize(); i++) {
            if (sType->at(i)->is<IR::Type_HeaderUnion>()) {
                auto stackLoc = read->getIndex(i);
                if (isHeaderUnionUninitialized(sType->at(i), currentDefinitions, stackLoc)) {
                    return true;
                }
            } else {
                return true;
            }
        }
        return false;
    }

    // For the following we compute the read set and save it.
    // We check the read set later.
    bool preorder(const IR::PathExpression* expression) override {
        LOG3("FU Visiting [" << expression->id << "]: " << expression);
        if (lhs) {
            reads(expression, LocationSet::empty);
            return false;
        }
        auto decl = refMap->getDeclaration(expression->path, true);
        LOG4("Declaration for path '" << expression->path << "' is "
            << IndentCtl::indent << IndentCtl::endl << decl
            << IndentCtl::unindent);

        auto storage = definitions->storageMap->getStorage(decl);
        const LocationSet* result;
        if (storage != nullptr)
            result = new LocationSet(storage);
        else
            result = LocationSet::empty;

        LOG4("LocationSet for declaration " << IndentCtl::indent << IndentCtl::endl << decl
            << IndentCtl::unindent << IndentCtl::endl << "is <<" << result << ">>");
        reads(expression, result);
        registerUses(expression);
        return false;
    }

    bool preorder(const IR::P4Action* action) override {
        BUG_CHECK(findContext<IR::P4Program>() == nullptr, "Unexpected action");
        LOG3("FU Visiting action " << action);
        unreachable = false;
        currentPoint = ProgramPoint(context, action);
        visit(action->body);
        checkOutParameters(action, action->parameters, getCurrentDefinitions());
        LOG3("FU Returning from " << action);
        return false;
    }

    bool preorder(const IR::P4Table* table) override {
        LOG3("FU Visiting " << table->name);
        auto savePoint = ProgramPoint(context, table);
        currentPoint = savePoint;
        auto saveHeaderDefsBeforeKey = headerDefs->clone();
        auto key = table->getKey();
        visit(key);
        auto saveHeaderDefsAfterKey = headerDefs->clone();
        HeaderDefinitions* finalHeaderDefs = nullptr;
        auto actions = table->getActionList();
        for (auto ale : actions->actionList) {
            BUG_CHECK(ale->expression->is<IR::MethodCallExpression>(),
                      "%1%: unexpected entry in action list", ale);
            headerDefs = saveHeaderDefsAfterKey->clone();
            visit(ale->expression);
            currentPoint = savePoint;  // restore the current point
                                    // it is modified by the inter-procedural analysis
            if (finalHeaderDefs) {
                finalHeaderDefs = finalHeaderDefs->intersect(headerDefs);
            } else {
                finalHeaderDefs = headerDefs;
            }
        }
        if (finalHeaderDefs) {
            headerDefs = finalHeaderDefs;
        }
        headerDefs->setNotReport(saveHeaderDefsBeforeKey);
        LOG3("FU Returning from " << table->name);
        return false;
    }

    void reportWarningIfInvalidHeader(const IR::Expression* expression) {
        if (!reportInvalidHeaders)
            return;

        LOG3("Checking if [" << expression->id << "]: " << expression << " is valid");
        auto valid = headerDefs->find(expression);
        if (valid == TernaryBool::No) {
            LOG3("accessing a field of an invalid header ["
                 << expression->id << "]: " << expression);
            warn(ErrorType::WARN_INVALID_HEADER,
                      "accessing a field of an invalid header %1%", expression);
        } else if (valid == TernaryBool::Maybe) {
            LOG3("accessing a field of a potentially invalid header ["
                 << expression->id << "]: " << expression);
            warn(ErrorType::WARN_INVALID_HEADER,
                      "accessing a field of a potentially invalid header %1%", expression);
        } else {
            LOG3("acessing a field of a valid header ["
                 << expression->id << "]: " << expression);
        }
    }

    bool preorder(const IR::MethodCallExpression* expression) override {
        LOG3("FU Visiting [" << expression->id << "]: " << expression);
        visit(expression->method);
        auto mi = MethodInstance::resolve(expression, refMap, typeMap);
        if (auto bim = mi->to<BuiltInMethod>()) {
            auto base = getReads(bim->appliedTo, true);
            cstring name = bim->name.name;
            if (name == IR::Type_Stack::push_front ||
                name == IR::Type_Stack::pop_front) {
                // Reads all array fields
                reads(expression, base);
                registerUses(expression, false);
                auto storage = headerDefs->getStorageLocation(bim->appliedTo);
                for (auto s : *storage)
                    headerDefs->setValueToStorage(s, TernaryBool::Yes);
                return false;
            } else if (name == IR::Type_Header::isValid) {
                auto storage = base->getField(StorageFactory::validFieldName);
                reads(expression, storage);
                registerUses(expression);
                headerDefs->addToNotReport(bim->appliedTo);
                return false;
            } else if (name == IR::Type_Header::setValid) {
                headerDefs->update(bim->appliedTo, TernaryBool::Yes);
            } else if (name == IR::Type_Header::setInvalid) {
                headerDefs->update(bim->appliedTo, TernaryBool::No);
            }
        }

        // The effect of copy-in: in arguments are read
        LOG3("Summarizing call effect on in arguments; definitions are " << IndentCtl::endl <<
             getCurrentDefinitions());

        bool isControlOrParserApply = false;
        if (mi->isApply()) {
            auto am = mi->to<ApplyMethod>();
            isControlOrParserApply = !am->isTableApply();
        }
        for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
            auto expr = mi->substitution.lookup(p);
            if (p->direction != IR::Direction::Out) {
                visit(expr);
            }

            // We assume control and parser apply calls and
            // extern methods set all output headers to valid
            if (isControlOrParserApply || mi->is<ExternMethod>() || mi->is<ExternFunction>())
                continue;

            if (auto actionCall = mi->to<ActionCall>()) {
                if (auto param = actionCall->action->parameters->getParameter(p->name)) {
                    if (p->direction == IR::Direction::Out) {
                        headerDefs->setValueToStorage(definitions->storageMap->getStorage(param),
                                                      TernaryBool::No);
                    } else {
                        // we can treat the argument passing as an assignment
                        auto param_expr = DeclarationToExpression::getInstance()->
                                          getExpression(param, refMap, typeMap);
                        processHeadersInAssignment(param_expr, expr->expression,
                                                   typeMap->getType(param_expr, true),
                                                   typeMap->getType(expr->expression, true));
                    }
                }
            }
        }

        // directionless parameters are set by the control-plane for actions invoked by tables
        if (auto actionCall = mi->to<ActionCall>()) {
            for (auto p : actionCall->action->parameters->parameters) {
                if (p->direction == IR::Direction::None && !mi->substitution.contains(p)) {
                    headerDefs->setValueToStorage(definitions->storageMap->getStorage(p),
                                                  TernaryBool::Yes);
                }
            }
        }

        // Symbolically call some methods (actions and tables, extern methods)
        std::vector <const IR::IDeclaration *> callee;
        if (auto ac = mi->to<ActionCall>()) {
            callee.push_back(ac->action);
        } else if (mi->isApply()) {
            auto am = mi->to<ApplyMethod>();
            if (am->isTableApply()) {
                auto table = am->object->to<IR::P4Table>();
                callee.push_back(table);
            }
        } else if (auto em = mi->to<ExternMethod>()) {
            LOG4("##call to extern " << expression);
            callee = em->mayCall(); }

        // We skip control and function apply calls, since we can
        // summarize their effects by assuming they write all out
        // parameters and read all in parameters and have no other
        // side effects.

        if (!callee.empty()) {
            LOG3("Analyzing " << callee << IndentCtl::indent);
            ProgramPoint pt(context, expression);
            FindUninitialized fu(this, pt);
            fu.setCalledBy(this);
            for (auto c : callee)
                (void)c->getNode()->apply(fu);
        }
        for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
            auto expr = mi->substitution.lookup(p);
            if (p->direction == IR::Direction::Out ||
                p->direction == IR::Direction::InOut) {
                bool save = lhs;
                lhs = true;
                visit(expr);
                lhs = save;

                if (isControlOrParserApply || mi->is<ExternMethod>() || mi->is<ExternFunction>()) {
                    if (typeMap->getType(expr->expression, true)->is<IR::Type_Header>()) {
                        headerDefs->update(expr->expression, TernaryBool::Yes);
                    } else {
                        auto locations = headerDefs->getStorageLocation(expr->expression);
                        for (auto storage : *locations) {
                            headerDefs->setValueToStorage(storage, TernaryBool::Yes);
                        }
                    }
                    continue;
                }

                if (auto actionCall = mi->to<ActionCall>()) {
                    if (auto param = actionCall->action->parameters->getParameter(p->name)) {
                        auto param_expr = DeclarationToExpression::getInstance()->
                                          getExpression(param, refMap, typeMap);
                        processHeadersInAssignment(expr->expression, param_expr,
                                                   typeMap->getType(expr->expression, true),
                                                   typeMap->getType(param_expr, true));
                    }
                }
            }
        }

        reads(expression, LocationSet::empty);
        return false;
    }

    bool preorder(const IR::Member* expression) override {
        LOG3("FU Visiting [" << expression->id << "]: " << expression);
        visit(expression->expr);
        LOG3("FU Returned from " << expression->expr);
        if (expression->expr->is<IR::TypeNameExpression>()) {
            // this is a constant
            reads(expression, LocationSet::empty);
            return false;
        }
        if (TableApplySolver::isHit(expression, refMap, typeMap) ||
            TableApplySolver::isActionRun(expression, refMap, typeMap))
            return false;

        auto type = typeMap->getType(expression, true);
        if (type->is<IR::Type_Method>())
            // dealt within the parent
            return false;

        auto storage = getReads(expression->expr, true);

        auto basetype = typeMap->getType(expression->expr, true);
        if (basetype->is<IR::Type_Stack>()) {
            if (expression->member.name == IR::Type_Stack::next ||
                expression->member.name == IR::Type_Stack::last) {
                // Accessing these fields implies reading the whole stack
                auto save  = lhs;
                lhs = false;
                visit(expression->expr);
                storage = getReads(expression->expr, true);
                lhs = save;
                reads(expression, storage);
                registerUses(expression, false);
                if (!lhs && expression->member.name == IR::Type_Stack::next)
                    warn(ErrorType::WARN_UNINITIALIZED,
                         "%1%: reading uninitialized value", expression);
                return false;
            } else if (expression->member.name == IR::Type_Stack::lastIndex) {
                auto index = storage->getArrayLastIndex();
                reads(expression, index);
                registerUses(expression, false);
                return false;
            }
        } else if (basetype->is<IR::Type_Header>()) {
            reportWarningIfInvalidHeader(expression->expr);
        }

        auto fields = storage->getField(expression->member);
        reads(expression, fields);
        registerUses(expression);
        return false;
    }

    bool preorder(const IR::Slice* expression) override {
        LOG3("FU Visiting [" << expression->id << "]: " << expression);

        auto* slice_stmt = findContext<IR::AssignmentStatement>();
        if (slice_stmt != nullptr && lhs) {
            // track this slice statement
            hasUses->watchForOverwrites(expression);
            LOG4("Tracking " << dbp(slice_stmt) << " " << slice_stmt <<
                    " for potential overwrites"); }

        bool save = lhs;
        lhs = false;  // slices on the LHS also read the data
        visit(expression->e0);
        LOG3("FU Returned from " << expression);
        auto storage = getReads(expression->e0, true);
        reads(expression, storage);   // true even in LHS
        registerUses(expression);
        lhs = save;

        hasUses->doneWatching();
        return false;
    }

    void otherExpression(const IR::Expression* expression) {
        BUG_CHECK(!lhs, "%1%: unexpected operation on LHS", expression);
        LOG3("FU Visiting [" << expression->id << "]: " << expression);
        // This expression in fact reads the result of the operation,
        // which is a temporary storage location, which we do not model
        // in the def-use analysis.
        reads(expression, LocationSet::empty);
        registerUses(expression);
    }

    void postorder(const IR::Mux* expression) override {
        otherExpression(expression);
    }

    bool preorder(const IR::ArrayIndex* expression) override {
        LOG3("FU Visiting [" << expression->id << "]: " << expression);
        if (auto cst = expression->right->to<IR::Constant>()) {
            if (lhs) {
                reads(expression, LocationSet::empty);
            } else {
                auto index = cst->asInt();
                visit(expression->left);
                auto storage = getReads(expression->left, true);
                auto result = storage->getIndex(index);
                reads(expression, result);
            }
        } else {
            // We model a write with an unknown index as a read/write
            // to the whole array.
            auto save = lhs;
            lhs = false;
            visit(expression->right);
            visit(expression->left);
            auto storage = getReads(expression->left, true);
            lhs = save;
            reads(expression, storage);
        }
        registerUses(expression);
        return false;
    }

    void postorder(const IR::Operation_Unary* expression) override {
        otherExpression(expression);
    }

    void postorder(const IR::Operation_Binary* expression) override {
        otherExpression(expression);
    }
};

class RemoveUnused : public Transform {
    const HasUses* hasUses;
    ReferenceMap*   refMap;
    TypeMap*        typeMap;

 public:
    explicit RemoveUnused(const HasUses* hasUses, ReferenceMap* refMap, TypeMap* typeMap)
                        : hasUses(hasUses), refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(hasUses);  CHECK_NULL(refMap);  CHECK_NULL(typeMap); setName("RemoveUnused"); }
    const IR::Node* postorder(IR::AssignmentStatement* statement) override {
        if (!hasUses->hasUses(getOriginal())) {
            LOG3("Removing statement " << getOriginal() << " " << statement << IndentCtl::indent);
            SideEffects se(refMap, typeMap);
            se.setCalledBy(this);
            (void)statement->right->apply(se);

            if (se.nodeWithSideEffect != nullptr) {
                // We expect that at this point there can't be more than 1
                // method call expression in each statement.
                BUG_CHECK(se.sideEffectCount == 1, "%1%: too many side-effect in one expression",
                          statement->right);
                BUG_CHECK(se.nodeWithSideEffect->is<IR::MethodCallExpression>(),
                          "%1%: expected method call", se.nodeWithSideEffect);
                auto mce = se.nodeWithSideEffect->to<IR::MethodCallExpression>();
                return new IR::MethodCallStatement(statement->srcInfo, mce);
            }
            // removing
            return new IR::EmptyStatement(statement->srcInfo);
        }
        return statement;
    }
    const IR::Node* postorder(IR::MethodCallStatement* mcs) override {
        if (!hasUses->hasUses(getOriginal())) {
            if (SideEffects::hasSideEffect(mcs->methodCall, refMap, typeMap)) {
                return mcs;
            }
            // removing
            LOG3("Removing statement " << getOriginal() << IndentCtl::indent);
            return new IR::EmptyStatement(mcs->srcInfo);
        }
        return mcs;
    }
};

// Run for each parser and control separately.
class ProcessDefUse : public PassManager {
    AllDefinitions *definitions;
    HasUses         hasUses;
 public:
    ProcessDefUse(ReferenceMap* refMap, TypeMap* typeMap) :
            definitions(new AllDefinitions(refMap, typeMap)) {
        passes.push_back(new ComputeWriteSet(definitions));
        passes.push_back(new FindUninitialized(definitions, &hasUses));
        passes.push_back(new RemoveUnused(&hasUses, refMap, typeMap));
        setName("ProcessDefUse");
    }
};
}  // namespace

const IR::Node* DoSimplifyDefUse::process(const IR::Node* node) {
    ProcessDefUse process(refMap, typeMap);
    process.setCalledBy(this);
    LOG5("ProcessDefUse of:" << IndentCtl::endl << node);
    return node->apply(process);
}

}  // namespace P4
