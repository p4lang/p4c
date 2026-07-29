/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_14_FROMV1_0_PROGRAMSTRUCTURE_H_
#define FRONTENDS_P4_14_FROMV1_0_PROGRAMSTRUCTURE_H_

#include <set>
#include <vector>

#include "frontends/p4/callGraph.h"
#include "frontends/p4/coreLibrary.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/map.h"
#include "v1model.h"

namespace P4::P4V1 {

class ConversionContext {
 public:
    virtual ~ConversionContext() = default;
    IR::Ptr<IR::Expression> header = nullptr;
    IR::Ptr<IR::Expression> userMetadata = nullptr;
    IR::Ptr<IR::Expression> standardMetadata = nullptr;
    virtual void clear() {
        header = nullptr;
        userMetadata = nullptr;
        standardMetadata = nullptr;
    }
};

/// Information about the structure of a P4-14 program, used to convert it to a P4-16 program.
class ProgramStructure {
    // In P4-14 one can have multiple objects with different types with the same name
    // In P4-16 this is not possible, so we may need to rename some objects.
    // We will preserve the original name using an @name("") annotation.
    template <typename T>
    class NamedObjectInfo {
        // If allNames is nullptr we don't check for duplicate names.
        // Key is a name, value represents how many times this name was used as a base
        // for newly generated unique names.
        std::unordered_map<cstring, int> *allNames;
        std::map<cstring, IR::Ptr<T>> nameToObject;
        std::map<IR::Ptr<T>, cstring> objectToNewName;

        // Iterate in order of name, but return pair<IR::Ptr<T>, newname>
        class iterator {
            friend class NamedObjectInfo;

         private:
            typename std::map<cstring, IR::Ptr<T>>::iterator it;
            typename std::map<IR::Ptr<T>, cstring> &objToName;
            iterator(typename std::map<cstring, IR::Ptr<T>>::iterator it,
                     typename std::map<IR::Ptr<T>, cstring> &objToName)
                : it(it), objToName(objToName) {}

         public:
            const iterator &operator++() {
                ++it;
                return *this;
            }
            bool operator!=(const iterator &other) const { return it != other.it; }
            std::pair<IR::Ptr<T>, cstring> operator*() const {
                return std::pair<IR::Ptr<T>, cstring>(it->second, objToName[it->second]);
            }
        };

     public:
        explicit NamedObjectInfo(std::unordered_map<cstring, int> *allNames) : allNames(allNames) {}
        void emplace(const T *obj) {
            if (objectToNewName.find(obj) != objectToNewName.end()) {
                // Already done
                LOG3(" already emplaced obj " << obj);
                return;
            }

            nameToObject.emplace(obj->name, obj);
            cstring newName;

            if (allNames == nullptr || (allNames->find(obj->name) == allNames->end())) {
                newName = obj->name;
            } else {
                newName = cstring::make_unique(*allNames, obj->name, allNames->at(obj->name), '_');
            }
            if (allNames != nullptr) allNames->insert({newName, 0});
            LOG3("Discovered " << obj << " named " << newName);
            objectToNewName.emplace(obj, newName);
        }
        /// Lookup using the original name
        IR::Ptr<T> get(cstring name) const { return ::P4::get(nameToObject, name); }
        /// Get the new name
        cstring get(const T *object) const {
            return ::P4::get(objectToNewName, object, object->name.name);
        }
        /// Get the new name from the old name
        cstring newname(cstring name) const { return get(get(name)); }
        bool contains(cstring name) const { return nameToObject.find(name) != nameToObject.end(); }
        iterator begin() { return iterator(nameToObject.begin(), objectToNewName); }
        iterator end() { return iterator(nameToObject.end(), objectToNewName); }
        void erase(cstring name) {
            allNames->erase(name);
            auto obj = get(name);
            objectToNewName.erase(obj);
            nameToObject.erase(name);
        }
    };

    std::set<cstring> included_files;

 public:
    ProgramStructure();
    virtual ~ProgramStructure() = default;

    P4V1::V1Model &v1model;
    P4::P4CoreLibrary &p4lib;

    std::unordered_map<cstring, int> allNames;
    NamedObjectInfo<IR::Type_StructLike> types;
    NamedObjectInfo<IR::HeaderOrMetadata> metadata;
    NamedObjectInfo<IR::Header> headers;
    NamedObjectInfo<IR::HeaderStack> stacks;
    NamedObjectInfo<IR::V1Control> controls;
    NamedObjectInfo<IR::V1Parser> parserStates;
    NamedObjectInfo<IR::V1Table> tables;
    NamedObjectInfo<IR::ActionFunction> actions;
    NamedObjectInfo<IR::Counter> counters;
    NamedObjectInfo<IR::Register> registers;
    NamedObjectInfo<IR::Meter> meters;
    NamedObjectInfo<IR::ActionProfile> action_profiles;
    NamedObjectInfo<IR::FieldList> field_lists;
    NamedObjectInfo<IR::FieldListCalculation> field_list_calculations;
    NamedObjectInfo<IR::ActionSelector> action_selectors;
    NamedObjectInfo<IR::Type_Extern> extern_types;
    std::map<const IR::Type_Extern *, const IR::Type_Extern *> extern_remap;
    NamedObjectInfo<IR::Declaration_Instance> externs;
    NamedObjectInfo<IR::ParserValueSet> value_sets;
    std::set<cstring> value_sets_implemented;
    std::vector<const IR::CalculatedField *> calculated_fields;
    std::map<const IR::Node *, const IR::Declaration_Instance *> globalInstances;
    P4::CallGraph<cstring> calledActions;
    P4::CallGraph<cstring> calledControls;
    P4::CallGraph<cstring> calledCounters;
    P4::CallGraph<cstring> calledMeters;
    P4::CallGraph<cstring> calledRegisters;
    P4::CallGraph<cstring> calledExterns;
    P4::CallGraph<cstring> parsers;
    std::map<cstring, IR::Vector<IR::Expression>> extracts;  // for each parser
    std::map<cstring, cstring> directCounters;               /// Maps table to direct counter.
    /// Maps table name to direct meter.
    std::map<cstring, const IR::Meter *> directMeters;
    std::map<const IR::Meter *, const IR::Declaration_Instance *> meterMap;
    std::map<cstring, const IR::Declaration_Instance *> counterMap;
    /// Field lists that appear in the program.
    ordered_set<const IR::FieldList *> allFieldLists;

    std::map<const IR::V1Table *, const IR::V1Control *> tableMapping;
    std::map<const IR::V1Table *, const IR::Apply *> tableInvocation;
    /// Some types are transformed during conversion; this maps the
    /// original P4-14 header type name to the final P4-16
    /// Type_Header.  We can't use the P4-14 type object itself as a
    /// key, because it keeps changing.
    std::map<cstring, const IR::Type *> finalHeaderType;
    /// For registers whose layout is a header, this map contains the mapping
    /// from the original layout type name to the final layout type name.
    std::map<cstring, cstring> registerLayoutType;

    /// Maps each inserted extract statement to the type of the header
    /// type that is being extracted.  The extracts will need another
    /// pass to cope with varbit fields.
    std::map<const IR::MethodCallExpression *, const IR::Type_Header *> extractsSynthesized;

    std::map<cstring, const IR::ParserState *> parserEntryPoints;
    /// Name of the serializable enum that holds one id for each field list.
    cstring fieldListsEnum;

    // P4-14 struct/header type can be converted to three types
    // of struct/header in P4-16.
    // 1) as part of the 'hdr' struct
    // 2) as part of the 'meta' struct
    // 3) as the parameters of a parser/control block
    // In case 1 and 2, the converter needs to fix the path
    // by prepending 'hdr.' or 'meta.' to the ConcreteHeaderRef.
    // In case 3. the converter only needs to convert headerRef to PathExpression
    std::set<cstring> headerTypes;
    std::set<cstring> metadataTypes;
    std::set<cstring> parameterTypes;
    std::set<cstring> metadataInstances;
    std::set<cstring> headerInstances;

    /// extra local instances to control created by primitive translation
    std::vector<const IR::Declaration *> localInstances;

    ConversionContext *conversionContext = nullptr;

    IR::Vector<IR::Type> *emptyTypeArguments = nullptr;
    const IR::Parameter *parserPacketIn = nullptr;
    const IR::Parameter *parserHeadersOut = nullptr;

    // output is constructed here
    IR::Vector<IR::Node> *declarations;

 protected:
    virtual IR::Ptr<IR::Statement> convertPrimitive(const IR::Primitive *primitive);
    virtual void checkHeaderType(const IR::Type_StructLike *hrd, bool toStruct);

    /**
     * Like addNameAnnotation(), but prefixes a "." to make the name global. You
     * should generally prefer this method; @see addNameAnnotation() for more
     * discussion.
     */
    static IR::Vector<IR::Annotation> addGlobalNameAnnotation(
        cstring name, const IR::Vector<IR::Annotation> &annos);

    virtual IR::Ptr<IR::ParserState> convertParser(const IR::V1Parser *,
                                                   IR::IndexedVector<IR::Declaration> *);
    virtual IR::Ptr<IR::Statement> convertParserStatement(const IR::Expression *expr);
    virtual IR::Ptr<IR::P4Control> convertControl(const IR::V1Control *control, cstring newName);
    virtual IR::Ptr<IR::Declaration_Instance> convertDirectMeter(const IR::Meter *m,
                                                                 cstring newName);
    virtual IR::Ptr<IR::Declaration_Instance> convertDirectCounter(const IR::Counter *c,
                                                                   cstring newName);
    virtual IR::Ptr<IR::Declaration_Instance> convert(const IR::CounterOrMeter *cm,
                                                      cstring newName);
    virtual IR::Ptr<IR::Declaration_Instance> convertActionProfile(const IR::ActionProfile *,
                                                                   cstring newName);
    virtual IR::Ptr<IR::P4Table> convertTable(const IR::V1Table *table, cstring newName,
                                              IR::IndexedVector<IR::Declaration> &stateful,
                                              std::map<cstring, cstring> &);
    virtual IR::Ptr<IR::P4Action> convertAction(const IR::ActionFunction *action, cstring newName,
                                                const IR::Meter *meterToAccess,
                                                cstring counterToAccess);
    virtual IR::Ptr<IR::Statement> convertMeterCall(const IR::Meter *meterToAccess);
    virtual IR::Ptr<IR::Statement> convertCounterCall(cstring counterToAccess);
    virtual IR::Ptr<IR::Type_Control> controlType(IR::ID name);
    const IR::PathExpression *getState(IR::ID dest);
    virtual const IR::Expression *counterType(const IR::CounterOrMeter *cm);
    virtual void createChecksumVerifications();
    virtual void createChecksumUpdates();
    virtual void createStructures();
    virtual cstring createType(const IR::Type_StructLike *type, bool header,
                               std::unordered_set<const IR::Type *> *converted);
    virtual void createParser();
    virtual void createControls();
    void createDeparserInternal(IR::ID deparserId, IR::Parameter *packetOut, IR::Parameter *headers,
                                std::vector<IR::Parameter *>,
                                IR::IndexedVector<IR::Declaration> controlLocals,
                                std::function<IR::BlockStatement *(IR::BlockStatement *)>);
    virtual void createDeparser();
    virtual void createMain();

 public:
    void include(cstring filename, cstring ppoptions = cstring());
    /// This inserts the names of the identifiers used in the output P4-16 programs
    /// into allNames, forcing P4-14 names that clash to be renamed.
    void populateOutputNames();
    IR::Ptr<IR::AssignmentStatement> assign(Util::SourceInfo srcInfo, const IR::Expression *left,
                                            const IR::Expression *right, const IR::Type *type);
    virtual IR::Ptr<IR::Expression> convertFieldList(const IR::Expression *expression);
    virtual IR::Ptr<IR::Expression> convertHashAlgorithm(Util::SourceInfo srcInfo,
                                                         IR::ID algorithm);
    virtual IR::Ptr<IR::Expression> convertHashAlgorithms(const IR::NameList *algorithm);
    virtual IR::Ptr<IR::Declaration_Instance> convert(const IR::Register *reg, cstring newName,
                                                      const IR::Type *regElementType = nullptr);
    virtual IR::Ptr<IR::Type_Struct> createFieldListType(const IR::Expression *expression);
    virtual IR::Ptr<IR::FieldListCalculation> getFieldListCalculation(const IR::Expression *);
    virtual IR::Ptr<IR::FieldList> getFieldLists(const IR::FieldListCalculation *flc);
    virtual IR::Ptr<IR::Expression> paramReference(const IR::Parameter *param);
    IR::Ptr<IR::Statement> sliceAssign(const IR::Primitive *prim, const IR::Expression *left,
                                       const IR::Expression *right, const IR::Expression *mask);
    void tablesReferred(const IR::V1Control *control, std::vector<const IR::V1Table *> &out);
    bool isHeader(const IR::ConcreteHeaderRef *nhr) const;
    cstring makeUniqueName(cstring base);
    bool isFieldInList(cstring type, cstring field, const IR::FieldList *fl) const;
    /// A vector with indexes of the field lists that contain this field.
    /// Returns nullptr if the field does not appear in any list.
    virtual const IR::Vector<IR::Expression> *listIndexes(cstring type, cstring field) const;
    /// Given an expression which is supposed to be a field list
    /// return a constant representing its value in the fieldListsEnum.
    IR::Ptr<IR::Expression> listIndex(const IR::Expression *fl) const;

    IR::Ptr<IR::Type> explodeType(const std::vector<const IR::Type::Bits *> &fieldTypes);
    IR::Ptr<IR::Expression> explodeLabel(const IR::Constant *value, const IR::Constant *mask,
                                         const std::vector<const IR::Type::Bits *> &fieldTypes);

    virtual IR::Ptr<IR::Vector<IR::Argument>> createApplyArguments(cstring n);

    IR::Ptr<IR::V1Control> ingress;
    IR::ID ingressReference;

    IR::Ptr<IR::P4Control> verifyChecksums;
    IR::Ptr<IR::P4Control> updateChecksums;
    IR::Ptr<IR::P4Control> deparser;
    /// Represents 'latest' P4-14 construct.
    IR::Ptr<IR::Expression> latest;
    const int defaultRegisterWidth = 32;

    virtual void loadModel();
    void createExterns();
    void createTypes();
    virtual IR::Ptr<IR::P4Program> create(Util::SourceInfo info);
};

}  // namespace P4::P4V1

#endif /* FRONTENDS_P4_14_FROMV1_0_PROGRAMSTRUCTURE_H_ */
