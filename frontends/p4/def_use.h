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

#ifndef FRONTENDS_P4_DEF_USE_H_
#define FRONTENDS_P4_DEF_USE_H_

#include "absl/container/flat_hash_set.h"
#include "absl/container/inlined_vector.h"
#include "absl/container/node_hash_set.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/ir.h"
#include "lib/alloc_trace.h"
#include "lib/flat_map.h"
#include "lib/hash.h"
#include "lib/hvec_map.h"
#include "lib/ordered_set.h"
#include "typeMap.h"

namespace P4 {

class ComputeWriteSet;
class StorageFactory;
class LocationSet;

// A location in the program. Includes the context from the visitor, which needs to
// be copied out of the Visitor::Context objects, as they are allocated on the stack and
// will become invalid as the IR traversal continues
struct loc_t {
    const IR::Node *node;
    const loc_t *parent;
    mutable std::size_t computedHash = 0;
    bool operator==(const loc_t &a) const {
        if (node != a.node) return false;
        if (parent == a.parent) return true;
        if (!parent || !a.parent) return false;
        return *parent == *a.parent;
    }
    std::size_t hash() const;
};

// #define DEBUG_LOCATION_IDS

/// Abstraction for something that is has a left value (variable, parameter)
class StorageLocation : public IHasDbPrint, public ICastable {
#ifdef DEBUG_LOCATION_IDS
    static unsigned crtid;
    unsigned id;
#endif

 public:
    virtual ~StorageLocation() {}
    const IR::Type *type;
    const cstring name;
    StorageLocation(const IR::Type *type, cstring name)
        :
#ifdef DEBUG_LOCATION_IDS
          id(crtid++),
#endif
          type(type),
          name(name) {
        CHECK_NULL(type);
    }
    void dbprint(std::ostream &out) const override {
#ifdef DEBUG_LOCATION_IDS
        out << id << " " << name;
#else
        out << dbp(type) << " " << name;
#endif
    }
    cstring toString() const { return name; }

    /// @returns All locations inside that represent valid bits.
    LocationSet getValidBits() const;
    virtual void addValidBits(LocationSet *result) const = 0;
    /// @returns All locations inside if we exclude all headers.
    LocationSet removeHeaders() const;
    virtual void removeHeaders(LocationSet *result) const = 0;
    /// @returns All locations inside that represent the 'lastIndex' of an array.
    LocationSet getLastIndexField() const;
    virtual void addLastIndexField(LocationSet *result) const = 0;

    DECLARE_TYPEINFO(StorageLocation);
};

/** Represents a storage location with a simple type or a tuple type.
    It could be either a scalar variable, or a field of a struct, etc. */
class BaseLocation : public StorageLocation {
 public:
    BaseLocation(const IR::Type *type, cstring name) : StorageLocation(type, name) {
        if (auto tt = type->to<IR::Type_Tuple>())
            BUG_CHECK(tt->getSize() == 0, "%1%: tuples with fields are not base locations", tt);
        else if (auto ts = type->to<IR::Type_StructLike>())
            BUG_CHECK(ts->fields.size() == 0, "%1%: structs with fields are not base locations",
                      tt);
        else
            BUG_CHECK(type->is<IR::Type_Bits>() || type->is<IR::Type_Enum>() ||
                          type->is<IR::Type_Boolean>() || type->is<IR::Type_Var>() ||
                          type->is<IR::Type_Error>() || type->is<IR::Type_Varbits>() ||
                          type->is<IR::Type_Newtype>() || type->is<IR::Type_SerEnum>() ||
                          type->is<IR::Type_List>(),
                      "%1%: unexpected type", type);
    }
    void addValidBits(LocationSet *) const override {}
    void addLastIndexField(LocationSet *) const override {}
    void removeHeaders(LocationSet *result) const override;

    DECLARE_TYPEINFO(BaseLocation, StorageLocation);
};

/// Base class for location sets that contain fields
class WithFieldsLocation : public StorageLocation {
    flat_map<cstring, const StorageLocation *, std::less<>,
             absl::InlinedVector<std::pair<cstring, const StorageLocation *>, 4>>
        fieldLocations;

    void createField(cstring name, StorageLocation *field) {
        fieldLocations.emplace(name, field);
        CHECK_NULL(field);
    }
    void replaceField(cstring field, StorageLocation *replacement) {
        fieldLocations[field] = replacement;
    }

    friend class StorageFactory;

 protected:
    WithFieldsLocation(const IR::Type *type, cstring name) : StorageLocation(type, name) {}

 public:
    void addField(cstring field, LocationSet *addTo) const;
    void removeHeaders(LocationSet *result) const override;
    auto fields() const { return Values(fieldLocations); }
    void dbprint(std::ostream &out) const override {
        for (auto f : fieldLocations) out << *f.second << " ";
    }

    DECLARE_TYPEINFO(WithFieldsLocation, StorageLocation);
};

/** Represents the locations for a struct, header or union */
class StructLocation : public WithFieldsLocation {
 protected:
    StructLocation(const IR::Type *type, cstring name) : WithFieldsLocation(type, name) {
        BUG_CHECK(type->is<IR::Type_StructLike>(), "%1%: unexpected type", type);
    }
    friend class StorageFactory;

 public:
    void addValidBits(LocationSet *result) const override;
    void addLastIndexField(LocationSet *result) const override;
    bool isHeader() const { return type->is<IR::Type_Header>(); }
    bool isHeaderUnion() const { return type->is<IR::Type_HeaderUnion>(); }
    bool isStruct() const { return type->is<IR::Type_Struct>(); }

    DECLARE_TYPEINFO(StructLocation, WithFieldsLocation);
};

/// Interface for locations that support an index operation
class IndexedLocation : public StorageLocation {
    absl::InlinedVector<const StorageLocation *, 8> elements;

 protected:
    void createElement(unsigned index, StorageLocation *element) {
        elements[index] = element;
        CHECK_NULL(element);
    }

    IndexedLocation(const IR::Type *type, cstring name) : StorageLocation(type, name) {
        CHECK_NULL(type);
        const auto *it = type->to<IR::Type_Indexed>();
        BUG_CHECK(it != nullptr, "%1%: unexpected type", type);
        elements.resize(it->getSize());
    }

    friend class StorageFactory;

 public:
    void addElement(unsigned index, LocationSet *result) const;
    auto begin() const { return elements.cbegin(); }
    auto end() const { return elements.cend(); }
    void dbprint(std::ostream &out) const override {
        for (unsigned i = 0; i < elements.size(); i++) out << *elements.at(i) << " ";
    }
    size_t getSize() const { return elements.size(); }
    void removeHeaders(LocationSet *result) const override;

    DECLARE_TYPEINFO(IndexedLocation, StorageLocation);
};

/** Represents the locations for a tuple or list */
class TupleLocation : public IndexedLocation {
 protected:
    TupleLocation(const IR::Type *type, cstring name) : IndexedLocation(type, name) {}
    friend class StorageFactory;

 public:
    void addValidBits(LocationSet *) const override {}
    void addLastIndexField(LocationSet *) const override {}

    DECLARE_TYPEINFO(TupleLocation, IndexedLocation);
};

class ArrayLocation : public IndexedLocation {
    const StorageLocation *lastIndexField = nullptr;  // accessed by lastIndex

    void setLastIndexField(const StorageLocation *location) { lastIndexField = location; }

 protected:
    ArrayLocation(const IR::Type *type, cstring name) : IndexedLocation(type, name) {}
    friend class StorageFactory;

 public:
    const StorageLocation *getLastIndexField() const { return lastIndexField; }

    void addValidBits(LocationSet *result) const override;
    void removeHeaders(LocationSet *) const override {}  // no results added
    void addLastIndexField(LocationSet *result) const override;

    DECLARE_TYPEINFO(ArrayLocation, IndexedLocation);
};

class StorageFactory {
    // FIXME: Allocate StorageLocations from an arena, not global allocator
    mutable std::vector<std::unique_ptr<StorageLocation>> storageLocations;

    template <class T>
    T *construct(const IR::Type *type, cstring name) const;

    static constexpr std::string_view indexFieldName = "$last_index";

 public:
    StorageLocation *create(const IR::Type *type, cstring name) const;

    static const cstring validFieldName;
};

/// A set of locations that may be read or written by a computation.
/// In general this is a conservative approximation of the actual location set.
class LocationSet : public IHasDbPrint {
    using LocationsStorage = ordered_set<const StorageLocation *>;
    LocationsStorage locations;

    class canonical_iterator {
        absl::InlinedVector<const StorageLocation *, 8> workList;

        void unwrapTop() {
            if (workList.empty()) return;

            const auto *location = workList.back();
            if (location->is<BaseLocation>()) return;

            if (const auto *wfl = location->to<WithFieldsLocation>()) {
                workList.pop_back();
                for (const auto *f : Util::iterator_range(wfl->fields()).reverse())
                    workList.push_back(f);
                unwrapTop();
                return;
            }

            if (const auto *a = location->to<IndexedLocation>()) {
                workList.pop_back();
                for (const auto *f : Util::iterator_range(*a).reverse()) workList.push_back(f);
                unwrapTop();
                return;
            }

            BUG("unexpected location");
        }

     public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const StorageLocation *;
        using difference_type = ptrdiff_t;
        using pointer = value_type;
        using reference = value_type;

        canonical_iterator() = default;

        explicit canonical_iterator(const LocationsStorage &locations) {
            for (const auto *loc : Util::iterator_range(locations).reverse())
                workList.push_back(loc);
            unwrapTop();
        }
        canonical_iterator &operator++() {
            workList.pop_back();
            unwrapTop();
            return *this;
        }
        canonical_iterator operator++(int) {
            auto copy = *this;
            ++*this;
            return copy;
        }
        bool operator==(const canonical_iterator &i) const { return workList == i.workList; }
        bool operator!=(const canonical_iterator &i) const { return workList != i.workList; }
        reference operator*() const { return workList.back(); }
        pointer operator->() const { return workList.back(); }
    };

 public:
    LocationSet() = default;
    explicit LocationSet(const ordered_set<const StorageLocation *> &other) : locations(other) {}
    explicit LocationSet(const StorageLocation *location) {
        CHECK_NULL(location);
        locations.emplace(location);
    }
    static const LocationSet *empty;

    const LocationSet *getField(cstring field) const;
    const LocationSet *getValidField() const;
    const LocationSet *getIndex(unsigned index) const;
    const LocationSet *allElements() const;
    const LocationSet *getArrayLastIndex() const;

    void add(const StorageLocation *location) {
        CHECK_NULL(location);
        locations.emplace(location);
    }
    const LocationSet *join(const LocationSet *other) const;
    /// @returns this location set expressed only in terms of BaseLocation;
    /// e.g., a StructLocation is expanded in all its fields.
    const LocationSet *canonicalize() const;
    void addCanonical(const StorageLocation *location);
    auto begin() const { return locations.cbegin(); }
    auto end() const { return locations.cend(); }
    auto canon_begin() const { return canonical_iterator(locations); }
    auto canon_end() const { return canonical_iterator(); }
    auto canonical() const { return Util::iterator_range(canon_begin(), canon_end()); }

    void dbprint(std::ostream &out) const override {
        if (locations.empty()) out << "LocationSet::empty";
        for (auto l : locations) {
            l->dbprint(out);
            out << " ";
        }
    }
    // only defined for canonical representations
    bool overlaps(const LocationSet *other) const;
    bool operator==(const LocationSet &other) const;
    bool isEmpty() const { return locations.empty(); }
};

/// Maps a declaration to its associated storage.
class StorageMap : public IHasDbPrint {
    /// Storage location for each declaration.
    hvec_map<const IR::IDeclaration *, const StorageLocation *> storage;
    StorageFactory factory;

 public:
    ReferenceMap *refMap;
    TypeMap *typeMap;

    StorageMap(ReferenceMap *refMap, TypeMap *typeMap) : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
    }
    const StorageLocation *add(const IR::IDeclaration *decl) {
        CHECK_NULL(decl);
        auto type = typeMap->getType(decl->getNode(), true);
        auto loc = factory.create(type, decl->getName() + "/" + decl->externalName());
        if (loc != nullptr) storage.emplace(decl, loc);
        return loc;
    }
    const StorageLocation *getOrAdd(const IR::IDeclaration *decl) {
        const auto *s = getStorage(decl);
        if (s != nullptr) return s;
        return add(decl);
    }
    const StorageLocation *getStorage(const IR::IDeclaration *decl) const {
        CHECK_NULL(decl);
        auto result = ::P4::get(storage, decl);
        return result;
    }
    void dbprint(std::ostream &out) const override {
        for (auto &it : storage) out << it.first << ": " << it.second << Log::endl;
    }
};

/// Indicates a statement in the program.
class ProgramPoint : public IHasDbPrint {
    /// The stack is for representing calls for context-sensitive analyses: i.e.,
    /// table.apply() -> table -> action.
    /// An empty stack represents "beforeStart"
    /// A nullptr on the stack represents a node *after* the termination of
    /// the previous context.  E.g., a stack [Function] is the context before
    /// the function, while [Function, nullptr] is the context after the
    /// function terminates.
    absl::InlinedVector<const IR::Node *, 8> stack;  // Has inline space for 8 nodes

 public:
    ProgramPoint() = default;
    ProgramPoint(const ProgramPoint &other) : stack(other.stack) {}
    explicit ProgramPoint(const IR::Node *node) {
        CHECK_NULL(node);
        assign(node);
    }
    ProgramPoint(const ProgramPoint &context, const IR::Node *node);
    /// A point logically before the function/control/action start.
    static ProgramPoint beforeStart;
    /// We use a nullptr to indicate a point *after* the previous context
    ProgramPoint after() { return ProgramPoint(*this, nullptr); }
    bool operator==(const ProgramPoint &other) const;
    std::size_t hash() const;
    void dbprint(std::ostream &out) const override {
        if (isBeforeStart()) {
            out << "<BeforeStart>";
        } else {
            bool first = true;
            for (auto n : stack) {
                if (!first) out << "//";
                if (!n)
                    out << "After end";
                else
                    out << dbp(n);
                first = false;
            }
            auto l = stack.back();
            if (l != nullptr &&
                (l->is<IR::AssignmentStatement>() || l->is<IR::MethodCallStatement>()))
                out << "[[" << l << "]]";
        }
    }
    void assign(const ProgramPoint &context, const IR::Node *node);
    void assign(const IR::Node *node) { stack.assign({node}); }
    void clear() { stack.clear(); }
    const IR::Node *last() const { return stack.empty() ? nullptr : stack.back(); }
    bool isBeforeStart() const { return stack.empty(); }
    auto begin() const { return stack.begin(); }
    auto end() const { return stack.end(); }
    ProgramPoint &operator=(const ProgramPoint &) = default;
    ProgramPoint &operator=(ProgramPoint &&) = default;
};
}  // namespace P4

// inject hash into std namespace so it is picked up by std::unordered_set
namespace std {
template <>
struct hash<P4::ProgramPoint> {
    std::size_t operator()(const P4::ProgramPoint &s) const { return s.hash(); }
};

template <>
struct hash<P4::loc_t> {
    std::size_t operator()(const P4::loc_t &loc) const { return loc.hash(); }
};

}  // namespace std

namespace P4::Util {
template <>
struct Hasher<P4::ProgramPoint> {
    size_t operator()(const P4::ProgramPoint &p) const { return p.hash(); }
};

template <>
struct Hasher<P4::loc_t> {
    size_t operator()(const P4::loc_t &loc) const { return loc.hash(); }
};
}  // namespace P4::Util

namespace P4 {
class ProgramPoints : public IHasDbPrint {
    typedef absl::flat_hash_set<ProgramPoint, Util::Hash> Points;
    Points points;
    explicit ProgramPoints(const Points &points) : points(points) {}

 public:
    ProgramPoints() = default;
    explicit ProgramPoints(ProgramPoint point) { points.emplace(point); }
    void add(const ProgramPoints *from);
    const ProgramPoints *merge(const ProgramPoints *with) const;
    bool operator==(const ProgramPoints &other) const;
    void dbprint(std::ostream &out) const override {
        out << "{";
        for (auto p : points) out << p << " ";
        out << "}";
    }
    size_t size() const { return points.size(); }
    bool containsBeforeStart() const {
        return points.find(ProgramPoint::beforeStart) != points.end();
    }
    Points::const_iterator begin() const { return points.cbegin(); }
    Points::const_iterator end() const { return points.cend(); }
};

/// List of definers for each base storage (at a specific program point).
class Definitions : public IHasDbPrint {
    /// Set of program points that have written last to each location
    /// (conservative approximation).
    hvec_map<const BaseLocation *, const ProgramPoints *> definitions;
    /// If true the current program point is actually unreachable.
    bool unreachable = false;

 public:
    Definitions() = default;
    Definitions(const Definitions &other)
        : definitions(other.definitions), unreachable(other.unreachable) {}
    Definitions *joinDefinitions(const Definitions *other) const;
    /// Point writes the specified LocationSet.
    Definitions *writes(ProgramPoint point, const LocationSet &locations) const;
    void setDefintion(const BaseLocation *loc, const ProgramPoints *point) {
        CHECK_NULL(loc);
        CHECK_NULL(point);
        definitions[loc] = point;
    }
    void setDefinition(const StorageLocation *loc, const ProgramPoints *point);
    void setDefinition(const LocationSet &loc, const ProgramPoints *point);
    Definitions *setUnreachable() {
        unreachable = true;
        return this;
    }
    bool isUnreachable() const { return unreachable; }
    bool hasLocation(const BaseLocation *location) const {
        return definitions.find(location) != definitions.end();
    }
    const ProgramPoints *getPoints(const BaseLocation *location) const {
        auto r = ::P4::get(definitions, location);
        BUG_CHECK(r != nullptr, "no definitions found for %1%", location);
        return r;
    }
    const ProgramPoints *getPoints(const LocationSet &locations) const;
    bool operator==(const Definitions &other) const;
    void dbprint(std::ostream &out) const override {
        if (unreachable) {
            out << "  Unreachable" << Log::endl;
        }
        if (definitions.empty()) out << "  Empty definitions";
        bool first = true;
        for (auto d : definitions) {
            if (!first) out << Log::endl;
            out << "  " << *d.first << "=>" << *d.second;
            first = false;
        }
    }
    Definitions *cloneDefinitions() const { return new Definitions(*this); }
    void removeLocation(const StorageLocation *loc);
    bool empty() const { return definitions.empty(); }
    size_t size() const { return definitions.size(); }
};

class AllDefinitions : public IHasDbPrint {
    /// These are the definitions available AFTER each ProgramPoint.
    /// However, for ProgramPoints representing P4Control, P4Action,
    /// P4Table, P4Function -- the definitions are BEFORE the
    /// ProgramPoint.
    hvec_map<ProgramPoint, Definitions *> atPoint;
    StorageMap storageMap;

 public:
    AllDefinitions(ReferenceMap *refMap, TypeMap *typeMap) : storageMap(refMap, typeMap) {}

    Definitions *getDefinitions(ProgramPoint point, bool emptyIfNotFound = false) {
        auto it = atPoint.find(point);
        if (it == atPoint.end()) {
            if (emptyIfNotFound) {
                auto defs = new Definitions();
                setDefinitionsAt(point, defs, false);
                return defs;
            }
            BUG("Unknown point %1% for definitions", &point);
        }
        return it->second;
    }
    void setDefinitionsAt(ProgramPoint point, Definitions *defs, bool overwrite) {
        if (!overwrite) {
            auto it = atPoint.find(point);
            if (it != atPoint.end()) {
                LOG2("Overwriting definitions at " << point << ": " << it->second << " with "
                                                   << defs);
                BUG_CHECK(false, "Overwriting definitions at %1%", point);
            }
        }
        atPoint[point] = defs;
    }

    const StorageLocation *getStorage(const IR::IDeclaration *decl) const {
        return storageMap.getStorage(decl);
    }

    const StorageLocation *getOrAddStorage(const IR::IDeclaration *decl) {
        return storageMap.getOrAdd(decl);
    }

    void dbprint(std::ostream &out) const override {
        for (auto e : atPoint) out << e.first << " => " << e.second << Log::endl;
    }
};

/**
 * Computes the write set for each expression and statement.
 *
 * This pass is run for each parser and control separately. It
 * controls precisely the visit order --- to simulate a symbolic
 * execution of the program.
 *
 * @pre Must be executed after variable initializers have been removed.
 *
 */

class ComputeWriteSet : public Inspector, public IHasDbPrint {
    // We are using node_hash_set here as we need pointers to entries (loc_t) to be stable
    using CachedLocs = absl::node_hash_set<loc_t, Util::Hash>;

 public:
    explicit ComputeWriteSet(AllDefinitions *allDefinitions, ReferenceMap *refMap, TypeMap *typeMap)
        : refMap(refMap),
          typeMap(typeMap),
          allDefinitions(allDefinitions),
          currentDefinitions(nullptr),
          returnedDefinitions(nullptr),
          exitDefinitions(new Definitions()),
          lhs(false),
          virtualMethod(false),
          cached_locs(new CachedLocs) {
        CHECK_NULL(allDefinitions);
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        visitDagOnce = false;
    }

    // expressions
    bool preorder(const IR::Literal *expression) override;
    bool preorder(const IR::AbstractSlice *expression) override;
    bool preorder(const IR::TypeNameExpression *expression) override;
    bool preorder(const IR::PathExpression *expression) override;
    bool preorder(const IR::Member *expression) override;
    bool preorder(const IR::ArrayIndex *expression) override;
    bool preorder(const IR::Operation_Binary *expression) override;
    bool preorder(const IR::Mux *expression) override;
    bool preorder(const IR::SelectExpression *expression) override;
    bool preorder(const IR::ListExpression *expression) override;
    bool preorder(const IR::Operation_Unary *expression) override;
    bool preorder(const IR::MethodCallExpression *expression) override;
    bool preorder(const IR::DefaultExpression *expression) override;
    bool preorder(const IR::Expression *expression) override;
    bool preorder(const IR::InvalidHeader *expression) override;
    bool preorder(const IR::InvalidHeaderUnion *expression) override;
    bool preorder(const IR::P4ListExpression *expression) override;
    bool preorder(const IR::HeaderStackExpression *expression) override;
    bool preorder(const IR::StructExpression *expression) override;
    // statements
    bool preorder(const IR::P4Parser *parser) override;
    bool preorder(const IR::P4Control *control) override;
    bool preorder(const IR::P4Action *action) override;
    bool preorder(const IR::P4Table *table) override;
    bool preorder(const IR::Function *function) override;
    bool preorder(const IR::AssignmentStatement *statement) override;
    bool preorder(const IR::ReturnStatement *statement) override;
    bool preorder(const IR::ExitStatement *statement) override;
    bool preorder(const IR::BreakStatement *statement) override;
    bool handleJump(const char *tok, Definitions *&defs);
    bool preorder(const IR::ContinueStatement *statement) override;
    bool preorder(const IR::IfStatement *statement) override;
    bool preorder(const IR::ForStatement *statement) override;
    bool preorder(const IR::ForInStatement *statement) override;
    bool preorder(const IR::BlockStatement *statement) override;
    bool preorder(const IR::SwitchStatement *statement) override;
    bool preorder(const IR::EmptyStatement *statement) override;
    bool preorder(const IR::MethodCallStatement *statement) override;

    const LocationSet *writtenLocations(const IR::Expression *expression) {
        expression->apply(*this);
        return getWrites(expression);
    }

 protected:
    ReferenceMap *refMap;
    TypeMap *typeMap;
    AllDefinitions *allDefinitions;              /// Result computed by this pass.
    Definitions *currentDefinitions;             /// Before statement currently processed.
    Definitions *returnedDefinitions;            /// Definitions after return statements.
    Definitions *exitDefinitions;                /// Definitions after exit statements.
    Definitions *breakDefinitions = nullptr;     /// Definitions at break statements.
    Definitions *continueDefinitions = nullptr;  /// Definitions at continue statements.
    ProgramPoint callingContext;
    /// if true we are processing an expression on the lhs of an assignment
    bool lhs;
    /// For each program location the location set it writes
    hvec_map<loc_t, const LocationSet *> writes;
    bool virtualMethod;  /// True if we are analyzing a virtual method
    AllocTrace memuse;
    alloc_trace_cb_t nested_trace;
    static int nest_count;

    /// Creates new visitor, but with same underlying data structures.
    /// Needed to visit some program fragments repeatedly.
    ComputeWriteSet(const ComputeWriteSet *source, ProgramPoint context, Definitions *definitions,
                    std::shared_ptr<CachedLocs> cached_locs)
        : refMap(source->refMap),
          typeMap(source->typeMap),

          allDefinitions(source->allDefinitions),
          currentDefinitions(definitions),
          returnedDefinitions(nullptr),
          exitDefinitions(source->exitDefinitions),
          breakDefinitions(source->breakDefinitions),
          continueDefinitions(source->continueDefinitions),
          callingContext(context),
          lhs(false),
          virtualMethod(false),
          cached_locs(cached_locs) {
        visitDagOnce = false;
    }
    void visitVirtualMethods(const IR::IndexedVector<IR::Declaration> &locals);
    const loc_t *getLoc(const IR::Node *n, const loc_t *parentLoc);
    const loc_t *getLoc(const Visitor::Context *ctxt);
    const loc_t *getLoc(const IR::Node *n, const Visitor::Context *ctxt);
    void enterScope(const IR::ParameterList *parameters,
                    const IR::IndexedVector<IR::Declaration> *locals, ProgramPoint startPoint,
                    bool clear = true);
    void exitScope(const IR::ParameterList *parameters,
                   const IR::IndexedVector<IR::Declaration> *locals, ProgramPoint endPoint);
    Definitions *getDefinitionsAfter(const IR::ParserState *state);
    bool setDefinitions(Definitions *defs, const IR::Node *who = nullptr, bool overwrite = false);
    ProgramPoint getProgramPoint(const IR::Node *node = nullptr) const;
    // Get writes of a node that is a direct child of the currently being visited node.
    const LocationSet *getWrites(const IR::Expression *expression) {
        const loc_t &exprLoc = *getLoc(expression, getChildContext());
        auto result = ::P4::get(writes, exprLoc);
        BUG_CHECK(result != nullptr, "No location set known for %1%", expression);
        return result;
    }
    // Get writes of a node that is not a direct child of the currently being visited node.
    // In this case, parentLoc is the loc of expression's direct parent node.
    const LocationSet *getWrites(const IR::Expression *expression, const loc_t *parentLoc) {
        const loc_t &exprLoc = *getLoc(expression, parentLoc);
        auto result = ::P4::get(writes, exprLoc);
        BUG_CHECK(result != nullptr, "No location set known for %1%", expression);
        return result;
    }
    // Register writes of expression, which is expected to be the currently visited node.
    void expressionWrites(const IR::Expression *expression, const LocationSet *loc) {
        CHECK_NULL(expression);
        CHECK_NULL(loc);
        LOG3(expression << dbp(expression) << " writes " << loc);
        const Context *ctx = getChildContext();
        BUG_CHECK(ctx->node == expression, "Expected ctx->node == expression.");
        const loc_t &exprLoc = *getLoc(ctx);
        if (auto it = writes.find(exprLoc); it != writes.end()) {
            BUG_CHECK(*it->second == *loc || expression->is<IR::Literal>(),
                      "Expression %1% write set already set", expression);
        } else {
            writes.emplace(exprLoc, loc);
        }
    }
    void dbprint(std::ostream &out) const override {
        if (writes.empty()) out << "No writes";
        for (auto &it : writes) out << it.first.node << " writes " << it.second << Log::endl;
    }
    profile_t init_apply(const IR::Node *root) override {
        auto rv = Inspector::init_apply(root);
        LOG1("starting ComputWriteSet" << Log::indent);
        if (nest_count++ == 0 && LOGGING(2)) {
            memuse.clear();
            nested_trace = memuse.start();
        }
        return rv;
    }
    void end_apply() override {
        LOG1("finished CWS" << Log::unindent);
        if (--nest_count == 0 && LOGGING(2)) {
            memuse.stop(nested_trace);
            LOG2(memuse);
        }
    }

 private:
    std::shared_ptr<CachedLocs> cached_locs;
};

}  // namespace P4

#endif /* FRONTENDS_P4_DEF_USE_H_ */
