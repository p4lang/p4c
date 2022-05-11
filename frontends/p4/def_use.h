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

#ifndef _FRONTENDS_P4_DEF_USE_H_
#define _FRONTENDS_P4_DEF_USE_H_

#include "lib/ordered_map.h"
#include "lib/ordered_set.h"
#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

class StorageFactory;
class LocationSet;

/// Abstraction for something that is has a left value (variable, parameter)
class StorageLocation : public IHasDbPrint {
    static unsigned crtid;

 public:
    virtual ~StorageLocation() {}
    unsigned id;
    const IR::Type* type;
    const cstring name;
    StorageLocation(const IR::Type* type, cstring name) : id(crtid++), type(type), name(name)
    { CHECK_NULL(type); }
    template <class T>
    const T* to() const {
        auto result = dynamic_cast<const T*>(this);
        return result;
    }
    template <class T>
    bool is() const {
        auto result = dynamic_cast<const T*>(this);
        return result != nullptr;
    }
    void dbprint(std::ostream& out) const override {
        out << id << " " << name;
    }
    cstring toString() const { return name; }

    /// @returns All locations inside that represent valid bits.
    const LocationSet* getValidBits() const;
    virtual void addValidBits(LocationSet* result) const = 0;
    /// @returns All locations inside if we exclude all headers.
    const LocationSet* removeHeaders() const;
    virtual void removeHeaders(LocationSet* result) const = 0;
    /// @returns All locations inside that represent the 'lastIndex' of an array.
    const LocationSet* getLastIndexField() const;
    virtual void addLastIndexField(LocationSet* result) const = 0;
};

/** Represents a storage location with a simple type or a tuple type.
    It could be either a scalar variable, or a field of a struct, etc. */
class BaseLocation : public StorageLocation {
 public:
    BaseLocation(const IR::Type* type, cstring name) :
            StorageLocation(type, name) {
        if (auto tt = type->to<IR::Type_Tuple>())
            BUG_CHECK(tt->getSize() == 0, "%1%: tuples with fields are not base locations", tt);
        else if (auto ts = type->to<IR::Type_StructLike>())
            BUG_CHECK(ts->fields.size() == 0,
                      "%1%: structs with fields are not base locations", tt);
        else
            BUG_CHECK(type->is<IR::Type_Bits>() || type->is<IR::Type_Enum>() ||
                      type->is<IR::Type_Boolean>() || type->is<IR::Type_Var>() ||
                      type->is<IR::Type_Error>() ||
                      type->is<IR::Type_Varbits>() || type->is<IR::Type_Newtype>() ||
                      type->is<IR::Type_SerEnum>() || type->is<IR::Type_List>(),
                      "%1%: unexpected type", type); }
    void addValidBits(LocationSet*) const override {}
    void addLastIndexField(LocationSet*) const override {}
    void removeHeaders(LocationSet* result) const override;
};

/// Base class for location sets that contain fields
class WithFieldsLocation : public StorageLocation {
 protected:
    ordered_map<cstring, const StorageLocation*> fieldLocations;
    friend class StorageFactory;
    WithFieldsLocation(const IR::Type* type, cstring name) :
            StorageLocation(type, name) {}
 public:
    void createField(cstring name, StorageLocation* field)
    { fieldLocations.emplace(name, field); CHECK_NULL(field); }
    void replaceField(cstring field, StorageLocation* replacement)
    { fieldLocations[field] = replacement; }
    IterValues<ordered_map<cstring, const StorageLocation*>::const_iterator> fields() const
    { return Values(fieldLocations); }
    void dbprint(std::ostream& out) const override {
        for (auto f : fieldLocations)
            out << *f.second << " ";
    }
};

/** Represents the locations for a struct, header or union */
class StructLocation : public WithFieldsLocation {
 public:
    StructLocation(const IR::Type* type, cstring name) :
            WithFieldsLocation(type, name) {
        BUG_CHECK(type->is<IR::Type_StructLike>(),
                  "%1%: unexpected type", type);
    }
    void addField(cstring field, LocationSet* addTo) const;
    void addValidBits(LocationSet* result) const override;
    void removeHeaders(LocationSet* result) const override;
    void addLastIndexField(LocationSet* result) const override;
    bool isHeader() const { return type->is<IR::Type_Header>(); }
    bool isHeaderUnion() const { return type->is<IR::Type_HeaderUnion>(); }
    bool isStruct() const { return type->is<IR::Type_Struct>(); }
};

/// Interface for locations that support an index operation
class IndexedLocation : public StorageLocation {
 protected:
    std::vector<const StorageLocation*> elements;
    friend class StorageFactory;

    void createElement(unsigned index, StorageLocation* element)
    { elements[index] = element; CHECK_NULL(element); }
 public:
    IndexedLocation(const IR::Type* type, cstring name) : StorageLocation(type, name) {
        CHECK_NULL(type);
        auto it = type->to<IR::Type_Indexed>();
        BUG_CHECK(it != nullptr, "%1%: unexpected type", type);
        elements.resize(it->getSize());
    }
    void addElement(unsigned index, LocationSet* result) const;
    std::vector<const StorageLocation*>::const_iterator begin() const
    { return elements.cbegin(); }
    std::vector<const StorageLocation*>::const_iterator end() const
    { return elements.cend(); }
};

/** Represents the locations for a tuple or list */
class TupleLocation : public IndexedLocation {
 public:
    TupleLocation(const IR::Type* type, cstring name) : IndexedLocation(type, name) {}
    size_t getSize() const { return elements.size(); }
    void addValidBits(LocationSet*) const override {}
    void addLastIndexField(LocationSet*) const override {}
    void removeHeaders(LocationSet* result) const override;
};

class ArrayLocation : public IndexedLocation {
    const StorageLocation* lastIndexField;  // accessed by lastIndex
 public:
    ArrayLocation(const IR::Type* type, cstring name) :
            IndexedLocation(type, name), lastIndexField(nullptr) {}
    void setLastIndexField(const StorageLocation* location)
    { lastIndexField = location; }
    const StorageLocation* getLastIndexField() const
    { return lastIndexField; }
    void dbprint(std::ostream& out) const override {
        for (unsigned i = 0; i < elements.size(); i++)
            out << *elements.at(i) << " ";
    }
    void addValidBits(LocationSet* result) const override;
    void removeHeaders(LocationSet*) const override {}  // no results added
    void addLastIndexField(LocationSet* result) const override;
};

class StorageFactory {
 public:
    StorageLocation* create(const IR::Type* type, cstring name) const;

    static const cstring validFieldName;
    static const cstring indexFieldName;
};

/// A set of locations that may be read or written by a computation.
/// In general this is a conservative approximation of the actual location set.
class LocationSet : public IHasDbPrint {
    ordered_set<const StorageLocation*> locations;

 public:
    LocationSet() = default;
    explicit LocationSet(const ordered_set<const StorageLocation*> &other) : locations(other) {}
    explicit LocationSet(const StorageLocation* location)
    { CHECK_NULL(location); locations.emplace(location); }
    static const LocationSet* empty;

    const LocationSet* getField(cstring field) const;
    const LocationSet* getValidField() const;
    const LocationSet* getIndex(unsigned index) const;
    const LocationSet* allElements() const;
    const LocationSet* getArrayLastIndex() const;

    void add(const StorageLocation* location)
    { CHECK_NULL(location); locations.emplace(location); }
    const LocationSet* join(const LocationSet* other) const;
    /// @returns this location set expressed only in terms of BaseLocation;
    /// e.g., a StructLocation is expanded in all its fields.
    const LocationSet* canonicalize() const;
    void addCanonical(const StorageLocation* location);
    ordered_set<const StorageLocation*>::const_iterator begin() const { return locations.cbegin(); }
    ordered_set<const StorageLocation*>::const_iterator end()   const { return locations.cend(); }
    void dbprint(std::ostream& out) const override {
        if (locations.empty())
            out << "LocationSet::empty";
        for (auto l : locations) {
            l->dbprint(out);
            out << " ";
        }
    }
    // only defined for canonical representations
    bool overlaps(const LocationSet* other) const;
    bool isEmpty() const { return locations.empty(); }
};

/// Maps a declaration to its associated storage.
class StorageMap : public IHasDbPrint {
    /// Storage location for each declaration.
    ordered_map<const IR::IDeclaration*, StorageLocation*> storage;
    StorageFactory factory;

 public:
    ReferenceMap*  refMap;
    TypeMap*       typeMap;

    StorageMap(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    StorageLocation* add(const IR::IDeclaration* decl) {
        CHECK_NULL(decl);
        auto type = typeMap->getType(decl->getNode(), true);
        auto loc = factory.create(type, decl->getName() + "/" + decl->externalName());
        if (loc != nullptr)
            storage.emplace(decl, loc);
        return loc;
    }
    StorageLocation* getOrAdd(const IR::IDeclaration* decl) {
        auto s = getStorage(decl);
        if (s != nullptr)
            return s;
        return add(decl);
    }
    StorageLocation* getStorage(const IR::IDeclaration* decl) const {
        CHECK_NULL(decl);
        auto result = ::get(storage, decl);
        return result;
    }
    void dbprint(std::ostream& out) const override {
        for (auto &it : storage)
            out << it.first << ": " << it.second << IndentCtl::endl;
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
    std::vector<const IR::Node*> stack;

 public:
    ProgramPoint() = default;
    ProgramPoint(const ProgramPoint& other) : stack(other.stack) {}
    explicit ProgramPoint(const IR::Node* node) { CHECK_NULL(node); stack.push_back(node); }
    ProgramPoint(const ProgramPoint& context, const IR::Node* node);
    /// A point logically before the function/control/action start.
    static ProgramPoint beforeStart;
    /// We use a nullptr to indicate a point *after* the previous context
    ProgramPoint after() { return ProgramPoint(*this, nullptr); }
    bool operator==(const ProgramPoint& other) const;
    std::size_t hash() const;
    void dbprint(std::ostream& out) const override {
        if (isBeforeStart()) {
            out << "<BeforeStart>";
        } else {
            bool first = true;
            for (auto n : stack) {
                if (!first)
                    out << "//";
                if (!n)
                    out << "After end";
                else
                    out << dbp(n);
                first = false;
            }
            auto l = stack.back();
            if (l != nullptr &&
                (l->is<IR::AssignmentStatement>() ||
                 l->is<IR::MethodCallStatement>()))
                out << "[[" << l << "]]";
        }
    }
    const IR::Node* last() const
    { return stack.empty() ? nullptr : stack.back(); }
    bool isBeforeStart() const
    { return stack.empty(); }
    std::vector<const IR::Node*>::const_iterator begin() const { return stack.begin(); }
    std::vector<const IR::Node*>::const_iterator end() const { return stack.end(); }
    ProgramPoint &operator=(const ProgramPoint &) = default;
    ProgramPoint &operator=(ProgramPoint &&) = default;
};
}  // namespace P4

// inject hash into std namespace so it is picked up by std::unordered_set
namespace std {
template<> struct hash<P4::ProgramPoint> {
    typedef P4::ProgramPoint argument_type;
    typedef std::size_t  result_type;
    result_type operator()(argument_type const &s) const {
        return s.hash();
    }
};
}  // namespace std

namespace P4 {
class ProgramPoints : public IHasDbPrint {
    typedef std::unordered_set<ProgramPoint> Points;
    Points points;
    explicit ProgramPoints(const Points &points) : points(points) {}
 public:
    ProgramPoints() = default;
    explicit ProgramPoints(ProgramPoint point) { points.emplace(point); }
    void add(ProgramPoint point) { points.emplace(point); }
    const ProgramPoints* merge(const ProgramPoints* with) const;
    bool operator==(const ProgramPoints& other) const;
    void dbprint(std::ostream& out) const override {
        out << "{";
        for (auto p : points)
            out << p << " ";
        out << "}";
    }
    size_t size() const { return points.size(); }
    bool containsBeforeStart() const
    { return points.find(ProgramPoint::beforeStart) != points.end(); }
    Points::const_iterator begin() const
    { return points.cbegin(); }
    Points::const_iterator end() const
    { return points.cend(); }
};

/// List of definers for each base storage (at a specific program point).
class Definitions : public IHasDbPrint {
    /// Set of program points that have written last to each location
    /// (conservative approximation).
    ordered_map<const BaseLocation*, const ProgramPoints*> definitions;
    /// If true the current program point is actually unreachable.
    bool unreachable;

 public:
    Definitions() = default;
    Definitions(const Definitions& other) :
            definitions(other.definitions), unreachable(other.unreachable) {}
    Definitions* joinDefinitions(const Definitions* other) const;
    /// Point writes the specified LocationSet.
    Definitions* writes(ProgramPoint point, const LocationSet* locations) const;
    void setDefintion(const BaseLocation* loc, const ProgramPoints* point)
    { CHECK_NULL(loc); CHECK_NULL(point); definitions[loc] = point; }
    void setDefinition(const StorageLocation* loc, const ProgramPoints* point);
    void setDefinition(const LocationSet* loc, const ProgramPoints* point);
    Definitions* setUnreachable() { unreachable = true; return this; }
    bool isUnreachable() const { return unreachable; }
    bool hasLocation(const BaseLocation* location) const
    { return definitions.find(location) != definitions.end(); }
    const ProgramPoints* getPoints(const BaseLocation* location) const {
        auto r = ::get(definitions, location);
        BUG_CHECK(r != nullptr, "no definitions found for %1%", location);
        return r; }
    const ProgramPoints* getPoints(const LocationSet* locations) const;
    bool operator==(const Definitions& other) const;
    void dbprint(std::ostream& out) const override {
        if (unreachable) {
            out << "  Unreachable" << IndentCtl::endl;
        }
        if (definitions.empty())
            out << "  Empty definitions";
        bool first = true;
        for (auto d : definitions) {
            if (!first)
                out << IndentCtl::endl;
            out << "  " << *d.first << "=>" << *d.second;
            first = false;
        }
    }
    Definitions* cloneDefinitions() const { return new Definitions(*this); }
    void removeLocation(const StorageLocation* loc);
    bool empty() const { return definitions.empty(); }
};

class AllDefinitions : public IHasDbPrint {
    /// These are the definitions available AFTER each ProgramPoint.
    /// However, for ProgramPoints representing P4Control, P4Action,
    /// P4Table, P4Function -- the definitions are BEFORE the
    /// ProgramPoint.
    std::unordered_map<ProgramPoint, Definitions*> atPoint;

 public:
    StorageMap* storageMap;
    AllDefinitions(ReferenceMap* refMap, TypeMap* typeMap) :
            storageMap(new StorageMap(refMap, typeMap)) {}
    Definitions* getDefinitions(ProgramPoint point, bool emptyIfNotFound = false) {
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
    void setDefinitionsAt(ProgramPoint point, Definitions* defs, bool overwrite) {
        if (!overwrite) {
            auto it = atPoint.find(point);
            if (it != atPoint.end()) {
                LOG2("Overwriting definitions at " << point << ": " <<
                     it->second << " with " << defs);
                BUG_CHECK(false, "Overwriting definitions at %1%", point);
            }
        }
        atPoint[point] = defs;
    }
    void dbprint(std::ostream& out) const override {
        for (auto e : atPoint)
            out << e.first << " => " << e.second << IndentCtl::endl;
    }
};

/**
 * Computes the write set for each expression and statement.
 *
 * This pass is run for each parser and control separately.  It
 * controls precisely the visit order --- to simulate a simbolic
 * execution of the program.
 *
 * @pre Must be executed after variable initializers have been removed.
 *
 */

class ComputeWriteSet : public Inspector, public IHasDbPrint {
 protected:
    AllDefinitions*     allDefinitions;  /// Result computed by this pass.
    Definitions*        currentDefinitions;  /// Before statement currently processed.
    Definitions*        returnedDefinitions;  /// Definitions after return statements.
    Definitions*        exitDefinitions;  /// Definitions after exit statements.
    ProgramPoint        callingContext;
    const StorageMap*   storageMap;
    /// if true we are processing an expression on the lhs of an assignment
    bool                lhs;
    /// For each expression the location set it writes
    ordered_map<const IR::Expression*, const LocationSet*> writes;
    bool                virtualMethod;  /// True if we are analyzing a virtual method

    /// Creates new visitor, but with same underlying data structures.
    /// Needed to visit some program fragments repeatedly.
    ComputeWriteSet(const ComputeWriteSet* source, ProgramPoint context, Definitions* definitions) :
            allDefinitions(source->allDefinitions), currentDefinitions(definitions),
            returnedDefinitions(nullptr), exitDefinitions(source->exitDefinitions),
            callingContext(context), storageMap(source->storageMap), lhs(false),
            virtualMethod(false) {
        visitDagOnce = false;
    }
    void visitVirtualMethods(const IR::IndexedVector<IR::Declaration> &locals);
    void enterScope(const IR::ParameterList* parameters,
                    const IR::IndexedVector<IR::Declaration>* locals,
                    ProgramPoint startPoint, bool clear = true);
    void exitScope(const IR::ParameterList* parameters,
                   const IR::IndexedVector<IR::Declaration>* locals);
    Definitions* getDefinitionsAfter(const IR::ParserState* state);
    bool setDefinitions(Definitions* defs, const IR::Node* who = nullptr, bool overwrite = false);
    ProgramPoint getProgramPoint(const IR::Node* node = nullptr) const;
    const LocationSet* getWrites(const IR::Expression* expression) const {
        auto result = ::get(writes, expression);
        BUG_CHECK(result != nullptr, "No location set known for %1%", expression);
        return result;
    }
    void expressionWrites(const IR::Expression* expression, const LocationSet* loc) {
        CHECK_NULL(expression); CHECK_NULL(loc);
        LOG3(expression << dbp(expression) << " writes " << loc);
        BUG_CHECK(writes.find(expression) == writes.end() || expression->is<IR::Literal>(),
                  "Expression %1% write set already set", expression);
        writes.emplace(expression, loc);
    }
    void dbprint(std::ostream& out) const override {
        if (writes.empty())
            out << "No writes";
        for (auto &it : writes)
            out << it.first << " writes " << it.second << IndentCtl::endl;
    }

 public:
    explicit ComputeWriteSet(AllDefinitions* allDefinitions) :
            allDefinitions(allDefinitions), currentDefinitions(nullptr),
            returnedDefinitions(nullptr), exitDefinitions(new Definitions()),
            storageMap(allDefinitions->storageMap), lhs(false), virtualMethod(false)
    { CHECK_NULL(allDefinitions); visitDagOnce = false; }

    // expressions
    bool preorder(const IR::Literal* expression) override;
    bool preorder(const IR::Slice* expression) override;
    bool preorder(const IR::TypeNameExpression* expression) override;
    bool preorder(const IR::PathExpression* expression) override;
    bool preorder(const IR::Member* expression) override;
    bool preorder(const IR::ArrayIndex* expression) override;
    bool preorder(const IR::Operation_Binary* expression) override;
    bool preorder(const IR::Mux* expression) override;
    bool preorder(const IR::SelectExpression* expression) override;
    bool preorder(const IR::ListExpression* expression) override;
    bool preorder(const IR::Operation_Unary* expression) override;
    bool preorder(const IR::MethodCallExpression* expression) override;
    bool preorder(const IR::DefaultExpression* expression) override;
    bool preorder(const IR::Expression* expression) override;
    // statements
    bool preorder(const IR::P4Parser* parser) override;
    bool preorder(const IR::P4Control* control) override;
    bool preorder(const IR::P4Action* action) override;
    bool preorder(const IR::P4Table* table) override;
    bool preorder(const IR::Function* function) override;
    bool preorder(const IR::AssignmentStatement* statement) override;
    bool preorder(const IR::ReturnStatement* statement) override;
    bool preorder(const IR::ExitStatement* statement) override;
    bool preorder(const IR::IfStatement* statement) override;
    bool preorder(const IR::BlockStatement* statement) override;
    bool preorder(const IR::SwitchStatement* statement) override;
    bool preorder(const IR::EmptyStatement* statement) override;
    bool preorder(const IR::MethodCallStatement* statement) override;

    const LocationSet* writtenLocations(const IR::Expression* expression) {
        expression->apply(*this);
        return getWrites(expression);
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_DEF_USE_H_ */
