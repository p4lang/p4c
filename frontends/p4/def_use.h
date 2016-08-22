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

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

class StorageFactory;

class StorageLocation {
  public:
    virtual ~StorageLocation() {}
    const IR::Type* type;
    StorageLocation(const IR::Type* type) : type(type)
    { CHECK_NULL(type); }
    template <class T>
    T* to() {
        auto result = dynamic_cast<T*>(this);
        CHECK_NULL(result);
        return result;
    }
    template <class T>
    const T* to() const {
        auto result = dynamic_cast<const T*>(this);
        CHECK_NULL(result);
        return result;
    }
    template <class T>
    bool is() const {
        auto result = dynamic_cast<const T*>(this);
        return result != nullptr;
    }
    virtual void dbprint(std::ostream&) const = 0;
};

/* Represents a storage location with a simple type.
   It could be either a variable, or a field of a struct,
   or even a header field from an array */
class BaseLocation : public StorageLocation {
 public:
    explicit BaseLocation(const IR::Type* type) : StorageLocation(type)
    { BUG_CHECK(type->is<IR::Type_Base>() || type->is<IR::Type_Enum>(),
                "%1%: unexpected type", type); }
    void dbprint(std::ostream&) const override {}
};

class StructLocation : public StorageLocation {
    std::map<cstring, StorageLocation*> fieldLocations;
    friend class StorageFactory;

    void addField(cstring name, StorageLocation* field)
    { fieldLocations.emplace(name, field); CHECK_NULL(field); }
 public:
    explicit StructLocation(const IR::Type* type) : StorageLocation(type) {
        BUG_CHECK(type->is<IR::Type_StructLike>(),
                  "%1%: unexpected type", type);
    }
    StorageLocation* getField(cstring field) const {
        auto result = ::get(fieldLocations, field);
        CHECK_NULL(result);
        return result;
    }
    IterValues<std::map<cstring, StorageLocation*>::iterator> fields()
    { return Values(fieldLocations); }
    void dbprint(std::ostream& out) const override {
        for (auto f : fieldLocations)
            out << f.first << " " << *f.second;
    }
};

class ArrayLocation : public StorageLocation {
    std::vector<StorageLocation*> elements;
    friend class StorageFactory;

    void addElement(unsigned index, StorageLocation* element)
    { elements[index] = element; CHECK_NULL(element); }
  public:
    explicit ArrayLocation(const IR::Type* type) : StorageLocation(type) {
        BUG_CHECK(type->is<IR::Type_Stack>(),
                  "%1%: unexpected type", type);
    }
    StorageLocation* getElement(unsigned index) const {
        BUG_CHECK(index < elements.size(), "%1%: out of bounds index", index);
        return elements.at(index);
    }
    std::vector<StorageLocation*>::const_iterator begin() const
    { return elements.cbegin(); }
    std::vector<StorageLocation*>::const_iterator end() const
    { return elements.cend(); }
    void dbprint(std::ostream& out) const override {
        for (unsigned i = 0; i < elements.size(); i++)
            out << i << " " << *elements.at(i);
    }
};

class StorageFactory {
    const TypeMap* typeMap;
 public:
    explicit StorageFactory(const TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); }
    StorageLocation* create(const IR::Type* type) const;
};

// A set of locations that may be read or written by a computation.
// In general this is a conservative approximation of the actual location set.
class LocationSet {
    std::set<StorageLocation*> locations;
 public:
    LocationSet() = default;
    explicit LocationSet(const std::set<StorageLocation*> &other) : locations(other) {}
    static const LocationSet* empty;
    const LocationSet* getField(cstring field) const;
    const LocationSet* getIndex(unsigned index) const;
    void add(StorageLocation* location)
    { locations.emplace(location); }
    const LocationSet* join(const LocationSet* other) const;
    // express this location set only in terms of BaseLocation;
    // e.g., a StructLocation is expanded in all its fields.
    const LocationSet* canonicalize() const;
    std::set<StorageLocation*>::const_iterator begin() const { return locations.cbegin(); }
    std::set<StorageLocation*>::const_iterator end()   const { return locations.cend(); }

 private:
    void addCanonical(StorageLocation* location);
};

// For each declaration we keep the associated storage
class StorageMap {
    std::map<const IR::IDeclaration*, StorageLocation*> storage;
    StorageFactory      factory;
 public:
    const ReferenceMap* refMap;
    const TypeMap*      typeMap;

    StorageMap(const ReferenceMap* refMap, const TypeMap* typeMap) :
            factory(typeMap), refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    void add(const IR::IDeclaration* decl) {
        CHECK_NULL(decl);
        auto type = typeMap->getType(decl->getNode(), true);
        auto loc = factory.create(type);
        if (loc != nullptr)
            storage.emplace(decl, loc);
    }
    StorageLocation* getStorage(const IR::IDeclaration* decl) const {
        CHECK_NULL(decl);
        auto result = ::get(storage, decl);
        CHECK_NULL(result);
        return result;
    }
    const LocationSet* writes(const IR::Expression* expression, bool lhs) const;
    const LocationSet* reads(const IR::Expression* expression, bool lhs) const;
};

// Indicates a statement in the program.
// The stack is for representing calls: i.e.,
// table.apply() statement -> table -> action statement
class ProgramPoint {
    std::vector<const IR::Node*> stack;
 public:
    ProgramPoint(const ProgramPoint& other) : stack(other.stack) {}
    ProgramPoint(const ProgramPoint& context, const IR::Node* node);
    bool operator==(const ProgramPoint& other) const;
    std::size_t hash() const;
    void dbprint(std::ostream& out) const
    { for (auto n : stack) out << dbp(n) << "/"; }
};
}

// hash and comparator for ProgramPoint
bool operator==(const P4::ProgramPoint& left, const P4::ProgramPoint& right) {
    return left.operator==(right);
}

// inject hash into std namespace so it is picked up by std::unordered_set
namespace std {
template<> struct hash<P4::ProgramPoint> {
    typedef P4::ProgramPoint argument_type;
    typedef std::size_t  result_type;
    result_type operator()(argument_type const &s) const {
        return s.hash();
    }
};
}

namespace P4 {
class ProgramPoints {
    typedef std::unordered_set<ProgramPoint> Points;
    Points points;
    ProgramPoints(const Points &points) : points(points) {}
 public:
    ProgramPoints() = default;
    void add(const ProgramPoint point) { points.emplace(point); }
    const ProgramPoints* merge(const ProgramPoints* with) const;
    bool operator==(const ProgramPoints& other) const;
    void dbprint(std::ostream& out) const {
        out << "{";
        for (auto p : points)
            out << p << " ";
        out << "}";
    }
};

// List of definers for each base storage (at a specific program point)
class Definitions {
    std::map<const BaseLocation*, const ProgramPoints*> definitions;
 public:
    Definitions() = default;
    Definitions(const Definitions& other) : definitions(other.definitions) {}
    Definitions* join(const Definitions* other) const;
    // point writes the specified LocationSet
    Definitions* writes(ProgramPoint point, const LocationSet* locations) const;
    void set(const BaseLocation* loc, const ProgramPoints* point)
    { CHECK_NULL(loc); CHECK_NULL(point); definitions[loc] = point; }
    bool operator==(const Definitions& other) const;
    void dbprint(std::ostream& out) const {
        for (auto d : definitions)
            out << *d.first << "=>" << *d.second << std::endl;
    }
};

class ComputeDefUse : public Inspector {
};

}  // namespace P4

#endif /* _FRONTENDS_P4_DEF_USE_H_ */
