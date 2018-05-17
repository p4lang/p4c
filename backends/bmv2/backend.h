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

#ifndef _BACKENDS_BMV2_BACKEND_H_
#define _BACKENDS_BMV2_BACKEND_H_

#include "analyzer.h"
#include "expression.h"
#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"
#include "helpers.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/json.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "JsonObjects.h"
#include "metermap.h"
#include "midend/convertEnums.h"
#include "midend/actionSynthesis.h"
#include "midend/removeLeftSlices.h"
#include "sharedActionSelectorCheck.h"
#include "options.h"

namespace BMV2 {

class ExpressionConverter;

// Backend is a the base class for SimpleSwitchBackend and PortableSwitchBackend.
class Backend {
    using DirectCounterMap = std::map<cstring, const IR::P4Table*>;

 public:
    BMV2Options&                     options;
    P4::ReferenceMap*                refMap;
    P4::TypeMap*                     typeMap;
    P4::ConvertEnums::EnumMapping*   enumMap;
    const IR::ToplevelBlock*         toplevel;
    ExpressionConverter*             conv;
    P4::P4CoreLibrary&               corelib;
    ProgramParts                     structure;
    Util::JsonObject                 jsonTop;
    DirectCounterMap                 directCounterMap;
    DirectMeterMap                   meterMap;

    BMV2::JsonObjects*               json;
    Util::JsonArray*                 counters;
    Util::JsonArray*                 externs;
    Util::JsonArray*                 field_lists;
    Util::JsonArray*                 learn_lists;
    Util::JsonArray*                 meter_arrays;
    Util::JsonArray*                 register_arrays;
    Util::JsonArray*                 force_arith;
    Util::JsonArray*                 field_aliases;

    std::set<cstring>                pipeline_controls;
    std::set<cstring>                non_pipeline_controls;
    std::set<cstring>                compute_checksum_controls;
    std::set<cstring>                verify_checksum_controls;
    std::set<cstring>                deparser_controls;

    std::set<cstring>                match_kinds;

    // map IR node to compile-time allocated resource blocks.
    std::map<const IR::Node*, const IR::CompileTimeValue*>  resourceMap;

    // bmv2 expects 'ingress' and 'egress' pipeline to have fixed name.
    // provide an map from user program block name to hard-coded names.
    std::map<cstring, cstring>       pipeline_namemap;

 public:
    Backend(BMV2Options& options, P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
            P4::ConvertEnums::EnumMapping* enumMap) :
        options(options),
        refMap(refMap), typeMap(typeMap), enumMap(enumMap),
        corelib(P4::P4CoreLibrary::instance),
        json(new BMV2::JsonObjects()) { refMap->setIsV1(options.isv1()); }
    void serialize(std::ostream& out) const { jsonTop.serialize(out); }
    P4::P4CoreLibrary &   getCoreLibrary() const   { return corelib; }
    ExpressionConverter * getExpressionConverter() { return conv; }
    DirectCounterMap &    getDirectCounterMap()    { return directCounterMap; }
    DirectMeterMap &      getMeterMap()  { return meterMap; }
    P4::ReferenceMap*     getRefMap()    { return refMap; }
    P4::TypeMap*          getTypeMap()   { return typeMap; }
    const IR::ToplevelBlock* getToplevelBlock() { CHECK_NULL(toplevel); return toplevel; }

    virtual void convert(const IR::ToplevelBlock* block, BMV2Options& options) = 0;
    virtual void convertExternObjects(Util::JsonArray *result, const P4::ExternMethod *em,
                              const IR::MethodCallExpression *mc, const IR::StatOrDecl *s,
                              const bool& emitExterns) = 0;
    virtual void convertExternFunctions(Util::JsonArray *result, const P4::ExternFunction *ef,
                                const IR::MethodCallExpression *mc, const IR::StatOrDecl* s) = 0;
    virtual void convertExternInstances(const IR::Declaration *c,
                                const IR::ExternBlock* eb, Util::JsonArray* action_profiles,
                                BMV2::SharedActionSelectorCheck& selector_check,
                                const bool& emitExterns) = 0;
    virtual void convertChecksum(const IR::BlockStatement* body, Util::JsonArray* checksums,
                                 Util::JsonArray* calculations, bool verify) = 0;
    /**
     * Returns the correct operation for performing an assignment in
     * the BMv2 JSON language depending on the type of data assigned.
     */
    static cstring jsonAssignment(const IR::Type* type, bool inParser);
};

/**
This class implements a policy suitable for the SynthesizeActions pass.
The policy is: do not synthesize actions for the controls whose names
are in the specified set.
For example, we expect that the code in the deparser will not use any
tables or actions.
*/
class SkipControls : public P4::ActionSynthesisPolicy {
    // set of controls where actions are not synthesized
    const std::set<cstring> *skip;

public:
    explicit SkipControls(const std::set<cstring> *skip) : skip(skip) { CHECK_NULL(skip); }
    bool convert(const IR::P4Control* control) const {
        if (skip->find(control->name) != skip->end())
            return false;
        return true;
    }
};

/**
This class implements a policy suitable for the RemoveComplexExpression pass.
The policy is: only remove complex expression for the controls whose names
are in the specified set.
For example, we expect that the code in ingress and egress will have complex
expression removed.
*/
class ProcessControls : public BMV2::RemoveComplexExpressionsPolicy {
    const std::set<cstring> *process;

public:
    explicit ProcessControls(const std::set<cstring> *process) : process(process) {
        CHECK_NULL(process);
    }
    bool convert(const IR::P4Control* control) const {
        if (process->find(control->name) != process->end())
            return true;
        return false;
    }
};

/**
This pass adds @name annotations to all fields of the user metadata
structure so that they do not clash with fields of the headers
structure.  This is necessary because both of them become global
objects in the output json.
*/
class RenameUserMetadata : public Transform {
    P4::ReferenceMap* refMap;
    const IR::Type_Struct* userMetaType;
    // Used as a prefix for the fields of the userMetadata structure
    // and also as a name for the userMetadata type clone.
    cstring namePrefix;

public:
    RenameUserMetadata(P4::ReferenceMap* refMap,
                       const IR::Type_Struct* userMetaType, cstring namePrefix):
        refMap(refMap), userMetaType(userMetaType), namePrefix(namePrefix)
    { setName("RenameUserMetadata"); CHECK_NULL(refMap); }

    const IR::Node* postorder(IR::Type_Struct* type) override {
        // Clone the user metadata type.  We want this type to be used
        // only for parameters.  For any other variables we will used
        // the clone we create.
        if (userMetaType != getOriginal())
            return type;

        auto vec = new IR::IndexedVector<IR::Node>();
        LOG2("Creating clone" << getOriginal());
        auto clone = type->clone();
        clone->name = namePrefix;
        vec->push_back(clone);

        // Rename all fields
        IR::IndexedVector<IR::StructField> fields;
        for (auto f : type->fields) {
            auto anno = f->getAnnotation(IR::Annotation::nameAnnotation);
            cstring suffix = "";
            if (anno != nullptr)
                suffix = IR::Annotation::getName(anno);
            if (suffix.startsWith(".")) {
                // We can't change the name of this field.
                // Hopefully the user knows what they are doing.
                fields.push_back(f->clone());
                continue;
            }

            if (!suffix.isNullOrEmpty())
                suffix = cstring(".") + suffix;
            else
                suffix = cstring(".") + f->name;
            cstring newName = namePrefix + suffix;
            auto stringLit = new IR::StringLiteral(newName);
            LOG2("Renaming " << f << " to " << newName);
            auto annos = f->annotations->addOrReplace(
                IR::Annotation::nameAnnotation, stringLit);
            auto field = new IR::StructField(f->srcInfo, f->name, annos, f->type);
            fields.push_back(field);
        }

        auto annotated = new IR::Type_Struct(
            type->srcInfo, type->name, type->annotations, std::move(fields));
        vec->push_back(annotated);
        return vec;
    }

    const IR::Node* preorder(IR::Type_Name* type) override {
        // Find any reference to the user metadata type that is used
        // (but not for parameters or the package instantiation)
        // and replace it with the cloned type.
        if (!findContext<IR::Declaration_Variable>())
            return type;
        auto decl = refMap->getDeclaration(type->path);
        if (decl == userMetaType)
            type->path = new IR::Path(type->path->srcInfo, IR::ID(type->path->srcInfo, namePrefix));
        LOG2("Replacing reference with " << type);
        return type;
    }
};

class BuildResourceMap : public Inspector {
public:
    Backend* backend;

    BuildResourceMap(Backend *backend) : backend(backend) {};

    bool preorder(const IR::ControlBlock* control) override {
        backend->resourceMap.emplace(control->container, control);
        for (auto cv : control->constantValue) {
            backend->resourceMap.emplace(cv.first, cv.second);
        }

        for (auto c : control->container->controlLocals) {
            if (c->is<IR::InstantiatedBlock>()) {
                backend->resourceMap.emplace(c, control->getValue(c));
            }
        }
        return false;
    }

    bool preorder(const IR::ParserBlock* parser) override {
        backend->resourceMap.emplace(parser->container, parser);
        for (auto cv : parser->constantValue) {
            backend->resourceMap.emplace(cv.first, cv.second);
            if (cv.second->is<IR::Block>()) {
                visit(cv.second->getNode());
            }
        }

        for (auto c : parser->container->parserLocals) {
            if (c->is<IR::InstantiatedBlock>()) {
                backend->resourceMap.emplace(c, parser->getValue(c));
            }
        }
        return false;
    }

    bool preorder(const IR::TableBlock* table) override {
        backend->resourceMap.emplace(table->container, table);
        for (auto cv : table->constantValue) {
            backend->resourceMap.emplace(cv.first, cv.second);
            if (cv.second->is<IR::Block>()) {
                visit(cv.second->getNode());
            }
        }
        return false;
    }

    bool preorder(const IR::PackageBlock* package) override {
        for (auto cv : package->constantValue) {
            if (cv.second->is<IR::Block>()) {
                visit(cv.second->getNode());
            }
        }
        return false;
    }

    bool preorder(const IR::ToplevelBlock* tlb) override {
        auto package = tlb->getMain();
        visit(package);
        return false;
    }
};

class ExtractMatchKind : public Inspector {
public:
    Backend *backend;
    ExtractMatchKind(Backend* backend) : backend(backend) {}
    bool preorder(const IR::Declaration_MatchKind* kind) override {
        for (auto member : kind->members) {
            backend->match_kinds.insert(member->name);
        }
        return false;
    }
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_BACKEND_H_ */
