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
#include "portableSwitch.h"
#include "simpleSwitch.h"

namespace BMV2 {

enum class Target { UNKNOWN_SWITCH, PORTABLE_SWITCH, SIMPLE_SWITCH };
using BMV2Context = P4CContextWithOptions<CompilerOptions>;

class ExpressionConverter;

class Backend : public PassManager {

    using DirectCounterMap = std::map<cstring, const IR::P4Table*>;

    // TODO(hanw): current implementation uses refMap and typeMap from midend.
    // Once all midend passes are refactored to avoid patching refMap, typeMap,
    // We can regenerated the refMap and typeMap in backend.
    P4::ReferenceMap*                refMap;
    P4::TypeMap*                     typeMap;
    P4::ConvertEnums::EnumMapping*   enumMap;
    const IR::ToplevelBlock*         toplevel;
    ExpressionConverter*             conv;
    P4::P4CoreLibrary&               corelib;
    ProgramParts                     structure;
    Util::JsonObject                 jsonTop;
    P4::PortableModel&               model;  // remove
    DirectCounterMap                 directCounterMap;
    DirectMeterMap                   meterMap;
    ErrorCodesMap                    errorCodesMap;

    // bmv2 backend supports multiple target architectures, we create different
    // json generators for each architecture to handle the differences in json
    // format for each architecture.
    P4V1::SimpleSwitch*              simpleSwitch;
    // PortableSwitchJsonConverter*  portableSwitch;

 public:
    BMV2::JsonObjects*               json;
    Target                           target;
    Util::JsonArray*                 counters;
    Util::JsonArray*                 externs;
    Util::JsonArray*                 field_lists;
    Util::JsonArray*                 learn_lists;
    Util::JsonArray*                 meter_arrays;
    Util::JsonArray*                 register_arrays;
    Util::JsonArray*                 force_arith;
    Util::JsonArray*                 field_aliases;

    // We place scalar user metadata fields (i.e., bit<>, bool)
    // in the scalarsName metadata object, so we may need to rename
    // these fields.  This map holds the new names.
    std::map<const IR::StructField*, cstring> scalarMetadataFields;

    std::set<cstring>                pipeline_controls;
    std::set<cstring>                non_pipeline_controls;
    std::set<cstring>                compute_checksum_controls;
    std::set<cstring>                verify_checksum_controls;
    std::set<cstring>                deparser_controls;

    // bmv2 expects 'ingress' and 'egress' pipeline to have fixed name.
    // provide an map from user program block name to hard-coded names.
    std::map<cstring, cstring>       pipeline_namemap;

 protected:
    ErrorValue retrieveErrorValue(const IR::Member* mem) const;
    void createFieldAliases(const char *remapFile);
    void genExternMethod(Util::JsonArray* result, P4::ExternMethod *em);

 public:
    Backend(bool isV1, P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
            P4::ConvertEnums::EnumMapping* enumMap) :
        refMap(refMap), typeMap(typeMap), enumMap(enumMap),
        corelib(P4::P4CoreLibrary::instance),
        model(P4::PortableModel::instance),
        simpleSwitch(new P4V1::SimpleSwitch(this)),
        json(new BMV2::JsonObjects()),
        target(Target::SIMPLE_SWITCH) { refMap->setIsV1(isV1); setName("BackEnd"); }
    void process(const IR::ToplevelBlock* block, CompilerOptions& options);
    void convert(CompilerOptions& options);
    void serialize(std::ostream& out) const
    { jsonTop.serialize(out); }
    P4::P4CoreLibrary &   getCoreLibrary() const   { return corelib; }
    ErrorCodesMap &       getErrorCodesMap()       { return errorCodesMap; }
    ExpressionConverter * getExpressionConverter() { return conv; }
    DirectCounterMap &    getDirectCounterMap()    { return directCounterMap; }
    DirectMeterMap &      getMeterMap()  { return meterMap; }
    P4::PortableModel &   getModel()     { return model; }
    ProgramParts &        getStructure() { return structure; }
    P4::ReferenceMap*     getRefMap()    { return refMap; }
    P4::TypeMap*          getTypeMap()   { return typeMap; }
    P4V1::SimpleSwitch*   getSimpleSwitch()        { return simpleSwitch; }
    const IR::ToplevelBlock* getToplevelBlock() { CHECK_NULL(toplevel); return toplevel; }
    /// True if this parameter represents the standard_metadata input.
    bool isStandardMetadataParameter(const IR::Parameter* param);

    /**
     * Returns the correct operation for performing an assignment in
     * the BMv2 JSON language depending on the type of data assigned.
     */
    static cstring jsonAssignment(const IR::Type* type, bool inParser);
    /// True if this parameter represents the user_metadata input.
    bool isUserMetadataParameter(const IR::Parameter* param);
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_BACKEND_H_ */
