/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _FRONTENDS_P4_FROMV1_0_PROGRAMSTRUCTURE_H_
#define _FRONTENDS_P4_FROMV1_0_PROGRAMSTRUCTURE_H_

#include <set>
#include <vector>

#include "lib/map.h"
#include "lib/cstring.h"
#include "frontends/p4/coreLibrary.h"
#include "ir/ir.h"
#include "frontends/p4/callGraph.h"
#include "v1model.h"

namespace P4V1 {

/// Information about the structure of a P4-14 program, used to convert it to a P4-16 program.
class ProgramStructure {
    // In P4-14 one can have multiple objects with different types with the same name
    // In P4-16 this is not possible, so we may need to rename some objects.
    // We will preserve the original name using an @name("") annotation.
    template<typename T>
    class NamedObjectInfo {
        // If allNames is nullptr we don't check for duplicate names
        std::unordered_set<cstring> *allNames;
        std::map<cstring, T> nameToObject;
        std::map<T, cstring> objectToNewName;

        // Iterate in order of name, but return pair<T, newname>
        class iterator {
            friend class NamedObjectInfo;
         private:
            typename std::map<cstring, T>::iterator it;
            typename std::map<T, cstring> &objToName;
            iterator(typename std::map<cstring, T>::iterator it,
                     typename std::map<T, cstring> &objToName) :
                    it(it), objToName(objToName) {}
         public:
            const iterator& operator++() { ++it; return *this; }
            bool operator!=(const iterator& other) const { return it != other.it; }
            std::pair<T, cstring> operator*() const {
                return std::pair<T, cstring>(it->second, objToName[it->second]);
            }
        };

     public:
        explicit NamedObjectInfo(std::unordered_set<cstring>* allNames) : allNames(allNames) {}
        void emplace(T obj) {
            if (objectToNewName.find(obj) != objectToNewName.end())
                // Already done
                return;

            nameToObject.emplace(obj->name, obj);
            cstring newName;

            if (allNames == nullptr ||
                (allNames->find(obj->name) == allNames->end())) {
                newName = obj->name;
            } else {
                newName = cstring::make_unique(*allNames, obj->name, '_');
            }
            if (allNames != nullptr)
                allNames->emplace(newName);
            LOG3("Discovered " << obj << " named " << newName);
            objectToNewName.emplace(obj, newName);
        }
        /// Lookup using the original name
        T get(cstring name) const { return ::get(nameToObject, name); }
        /// Get the new name
        cstring get(T object) const { return ::get(objectToNewName, object); }
        bool contains(cstring name) const { return nameToObject.find(name) != nameToObject.end(); }
        iterator begin() { return iterator(nameToObject.begin(), objectToNewName); }
        iterator end() { return iterator(nameToObject.end(), objectToNewName); }
    };
    class FixupExtern;

 public:
    ProgramStructure();

    P4V1::V1Model &v1model;
    P4::P4CoreLibrary &p4lib;

    std::unordered_set<cstring>                 allNames;
    NamedObjectInfo<const IR::Type_StructLike*> types;
    NamedObjectInfo<const IR::Metadata*>        metadata;
    NamedObjectInfo<const IR::Header*>          headers;
    NamedObjectInfo<const IR::HeaderStack*>     stacks;
    NamedObjectInfo<const IR::V1Control*>       controls;
    NamedObjectInfo<const IR::V1Parser*>        parserStates;
    NamedObjectInfo<const IR::V1Table*>         tables;
    NamedObjectInfo<const IR::ActionFunction*>  actions;
    NamedObjectInfo<const IR::Counter*>         counters;
    NamedObjectInfo<const IR::Register*>        registers;
    NamedObjectInfo<const IR::Meter*>           meters;
    NamedObjectInfo<const IR::ActionProfile*>   action_profiles;
    NamedObjectInfo<const IR::FieldList*>       field_lists;
    NamedObjectInfo<const IR::FieldListCalculation*> field_list_calculations;
    NamedObjectInfo<const IR::ActionSelector*>  action_selectors;
    NamedObjectInfo<const IR::Type_Extern *>    extern_types;
    NamedObjectInfo<const IR::Declaration_Instance *>  externs;
    std::vector<const IR::CalculatedField*>     calculated_fields;
    P4::CallGraph<cstring> calledActions;
    P4::CallGraph<cstring> calledControls;
    P4::CallGraph<cstring> calledCounters;
    P4::CallGraph<cstring> calledMeters;
    P4::CallGraph<cstring> calledRegisters;
    P4::CallGraph<cstring> calledExterns;
    P4::CallGraph<cstring> parsers;
    std::map<cstring, IR::Vector<IR::Expression>> extracts;  // for each parser
    std::map<cstring, cstring> directCounters;  /// Maps table to direct counter.
    /// Maps table name to direct meter.
    std::map<cstring, const IR::Meter*> directMeters;
    std::map<const IR::Meter*, const IR::Declaration_Instance*> meterMap;
    std::map<cstring, const IR::Declaration_Instance*> counterMap;

    std::map<const IR::V1Table*, const IR::V1Control*> tableMapping;
    std::map<const IR::V1Table*, const IR::Apply*> tableInvocation;
    /// Some types are transformed during conversion; this maps the
    /// original P4-14 header type name to the final P4-16
    /// Type_Header.  We can't use the P4-14 type object itself as a
    /// key, because it keeps changing.
    std::map<cstring, const IR::Type*> finalHeaderType;

    /// Maps each inserted extract statement to the type of the header
    /// type that is being extracted.  The extracts will need another
    /// pass to cope with varbit fields.
    std::map<const IR::MethodCallExpression*, const IR::Type_Header*> extractsSynthesized;

    struct ConversionContext {
        const IR::Expression* header;
        const IR::Expression* userMetadata;
        const IR::Expression* standardMetadata;
        void clear() {
            header = nullptr;
            userMetadata = nullptr;
            standardMetadata = nullptr;
        }
    };

    ConversionContext conversionContext;
    IR::Vector<IR::Type>* emptyTypeArguments;
    const IR::Parameter* parserPacketIn;
    const IR::Parameter* parserHeadersOut;

    // output is constructed here
    IR::IndexedVector<IR::Node>* declarations;

 protected:
    void include(cstring filename);
    const IR::Statement* convertPrimitive(const IR::Primitive* primitive);
    void checkHeaderType(const IR::Type_StructLike* hrd, bool toStruct);
    const IR::Annotations* addNameAnnotation(cstring name, const IR::Annotations* annos = nullptr);
    const IR::ParserState* convertParser(const IR::V1Parser* prs);
    const IR::Statement* convertParserStatement(const IR::Expression* expr);
    const IR::P4Control* convertControl(const IR::V1Control* control, cstring newName);
    const IR::Declaration_Instance* convertDirectMeter(const IR::Meter* m, cstring newName);
    const IR::Declaration_Instance* convertDirectCounter(const IR::Counter* m, cstring newName);
    const IR::Declaration_Instance* convert(const IR::CounterOrMeter* cm, cstring newName);
    const IR::Declaration_Instance* convert(const IR::Register* reg, cstring newName);
    const IR::Declaration_Instance*
    convertExtern(const IR::Declaration_Instance* ext, cstring newName);
    const IR::P4Table*
    convertTable(const IR::V1Table* table, cstring newName,
                 IR::IndexedVector<IR::Declaration> &stateful, std::map<cstring, cstring> &);
    const IR::P4Action* convertAction(const IR::ActionFunction* action, cstring newName,
                                      const IR::Meter* meterToAccess, cstring counterToAccess);
    const IR::Type_Control* controlType(IR::ID name);
    const IR::PathExpression* getState(IR::ID dest);
    const IR::Declaration_Instance* checksumUnit(const IR::FieldListCalculation* flc);
    const IR::FieldList* getFieldLists(const IR::FieldListCalculation* flc);
    const IR::Expression* convertFieldList(const IR::Expression* expression);
    const IR::Type_Struct* createFieldListType(const IR::Expression* expression);
    const IR::Expression* convertHashAlgorithm(IR::ID algorithm);
    const IR::Statement* sliceAssign(Util::SourceInfo srcInfo, const IR::Expression* left,
                                     const IR::Expression* right, const IR::Expression* mask);
    const IR::Expression* counterType(const IR::CounterOrMeter* cm) const;
    cstring mapAlgorithm(IR::ID algorithm) const;
    void createChecksumVerifications();
    void createChecksumUpdates();
    void createStructures();
    void createExterns();
    void createType(const IR::Type_StructLike* type, bool header,
                    std::unordered_set<const IR::Type*> *converted);
    void createTypes();
    void createParser();
    void createControls();
    void createDeparser();
    void createMain();

 public:
    const IR::AssignmentStatement* assign(Util::SourceInfo srcInfo, const IR::Expression* left,
                                          const IR::Expression* right, const IR::Type* type);
    const IR::Expression* paramReference(const IR::Parameter* param);
    void tablesReferred(const IR::V1Control* control, std::vector<const IR::V1Table*> &out);
    bool isHeader(const IR::ConcreteHeaderRef* nhr) const;
    cstring makeUniqueName(cstring base);

    const IR::V1Control* ingress;
    IR::ID ingressReference;

    const IR::P4Control* verifyChecksums;
    const IR::P4Control* updateChecksums;
    const IR::P4Control* deparser;
    /// Represents 'latest' P4-14 construct.
    const IR::Expression* latest;
    const int defaultRegisterWidth = 32;

    void loadModel();
    const IR::P4Program* create(Util::SourceInfo info);
};

}  // namespace P4V1

#endif /* _FRONTENDS_P4_FROMV1_0_PROGRAMSTRUCTURE_H_ */
