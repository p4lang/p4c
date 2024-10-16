/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "bfruntime_ext.h"

namespace BFN {

namespace BFRT {

static const p4configv1::Extern*
findExternType(const p4configv1::P4Info& p4info, ::barefoot::P4Ids::Prefix externTypeId) {
    for (const auto& externType : p4info.externs()) {
        if (externType.extern_type_id() == static_cast<uint32_t>(externTypeId))
            return &externType;
    }
    return nullptr;
}

static const p4configv1::ExternInstance*
findExternInstance(const p4configv1::P4Info& p4info, P4Id externId) {
    auto prefix = static_cast<::barefoot::P4Ids::Prefix>(getIdPrefix(externId));
    auto* externType = findExternType(p4info, prefix);
    if (externType == nullptr) return nullptr;
    auto* externInstance = Standard::findP4InfoObject(
        externType->instances().begin(), externType->instances().end(), externId);
    return externInstance;
}

static void addROSingleton(Util::JsonArray* dataJson, Util::JsonObject* dataField) {
    addSingleton(dataJson, dataField, false, true);
}

static Util::JsonObject* makeTypeFloat(cstring type) {
    auto* typeObj = new Util::JsonObject();
    typeObj->emplace("type"_cs, type);
    return typeObj;
}

static Util::JsonObject* makeTypeString(std::optional<cstring> defaultValue = std::nullopt) {
    auto* typeObj = new Util::JsonObject();
    typeObj->emplace("type"_cs, "string"_cs);
    if (defaultValue != std::nullopt)
        typeObj->emplace("default_value"_cs, *defaultValue);
    return typeObj;
}

static P4Id makeActProfId(P4Id implementationId) {
  return makeBFRuntimeId(implementationId, ::barefoot::P4Ids::ACTION_PROFILE);
}

static P4Id makeActSelectorId(P4Id implementationId) {
  return makeBFRuntimeId(implementationId, ::barefoot::P4Ids::ACTION_SELECTOR);
}

struct BFRuntimeSchemaGenerator::ActionSelector {
    std::string name;
    std::string get_mem_name;
    P4Id id;
    P4Id get_mem_id;
    P4Id action_profile_id;
    int64_t max_group_size;
    int64_t num_groups;  // aka size of selector
    int64_t adt_offset;
    std::vector<P4Id> tableIds;
    Util::JsonArray* annotations;

    static std::optional<ActionSelector>
    from(const p4configv1::P4Info& p4info, const p4configv1::ActionProfile& actionProfile) {
        const auto& pre = actionProfile.preamble();
        if (!actionProfile.with_selector())
            return std::nullopt;
        auto selectorId = makeBFRuntimeId(pre.id(), ::barefoot::P4Ids::ACTION_SELECTOR);
        auto selectorGetMemId = makeBFRuntimeId(pre.id(),
                ::barefoot::P4Ids::ACTION_SELECTOR_GET_MEMBER);
        auto tableIds = collectTableIds(
            p4info, actionProfile.table_ids().begin(), actionProfile.table_ids().end());
        return ActionSelector{pre.name(), pre.name() + "_get_member",
            selectorId, selectorGetMemId, actionProfile.preamble().id(),
            actionProfile.max_group_size(), actionProfile.size(), 0xdeadbeef /* adt_offset */,
            tableIds, transformAnnotations(pre)};
    }

    static std::optional<ActionSelector>
    fromTNA(const p4configv1::P4Info& p4info, const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::ActionSelector actionSelector;
        if (!externInstance.info().UnpackTo(&actionSelector)) {
            error("Extern instance %1% does not pack an ActionSelector object", pre.name());
            return std::nullopt;
        }
        auto selectorId = makeBFRuntimeId(pre.id(), ::barefoot::P4Ids::ACTION_SELECTOR);
        auto selectorGetMemId = makeBFRuntimeId(pre.id(),
                ::barefoot::P4Ids::ACTION_SELECTOR_GET_MEMBER);
        auto tableIds = collectTableIds(
            p4info, actionSelector.table_ids().begin(), actionSelector.table_ids().end());
        return ActionSelector{pre.name(), pre.name() + "_get_member",
            selectorId, selectorGetMemId, actionSelector.action_profile_id(),
            actionSelector.max_group_size(), actionSelector.num_groups(),
            0xdeadbeef /* adt_offset */, tableIds, transformAnnotations(pre)};
    };
};

/// This struct is meant to be used as a common representation of ValueSet
/// objects for both PSA & TNA programs. However, at the moment we only support
/// TNA ValueSet objects.
struct BFRuntimeSchemaGenerator::ValueSet {
    std::string name;
    P4Id id;
    p4configv1::P4DataTypeSpec typeSpec;
    const int64_t size;
    Util::JsonArray* annotations;

    static std::optional<ValueSet> fromTNA(const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::ValueSet valueSet;
        if (!externInstance.info().UnpackTo(&valueSet)) {
            error("Extern instance %1% does not pack a value set object", pre.name());
            return std::nullopt;
        }
        return ValueSet{pre.name(), pre.id(), valueSet.type_spec(), valueSet.size(),
                        transformAnnotations(pre)};
    }
};

// It is tempting to unify the code for Lpf & Wred (using CRTP?) but the
// resulting code doesn't look too good and is not really less verbose. In
// particular aggregate initialization is not possible when a class has a base.
struct BFRuntimeSchemaGenerator::Lpf {
    std::string name;
    P4Id id;
    int64_t size;
    Util::JsonArray* annotations;

    static std::optional<Lpf> fromTNA(
        const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::Lpf lpf;
        if (!externInstance.info().UnpackTo(&lpf)) {
            error("Extern instance %1% does not pack a Lpf object", pre.name());
            return std::nullopt;
        }
        return Lpf{pre.name(), pre.id(), lpf.size(), transformAnnotations(pre)};
    }

    static std::optional<Lpf> fromTNADirect(
        const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::DirectLpf lpf;
        if (!externInstance.info().UnpackTo(&lpf)) {
            error("Extern instance %1% does not pack a Lpf object", pre.name());
            return std::nullopt;
        }
        return Lpf{pre.name(), pre.id(), 0, transformAnnotations(pre)};
    }
};

struct BFRuntimeSchemaGenerator::RegisterParam {
    std::string name;
    std::string dataFieldName;
    P4Id id;
    P4Id tableId;
    int64_t initial_value;
    p4configv1::P4DataTypeSpec typeSpec;
    Util::JsonArray* annotations;

    static std::optional<RegisterParam>
    fromTNA(const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::RegisterParam register_param_;
        if (!externInstance.info().UnpackTo(&register_param_)) {
            error("Extern instance %1% does not pack a RegisterParam object", pre.name());
            return std::nullopt;
        }
        return RegisterParam{pre.name(),
                             register_param_.data_field_name(),
                             pre.id(),
                             register_param_.table_id(),
                             register_param_.initial_value(),
                             register_param_.type_spec(),
                             transformAnnotations(pre)};
    }
};

struct BFRuntimeSchemaGenerator::PortMetadata {
    P4Id id;
    std::string name;
    std::string key_name;
    p4configv1::P4DataTypeSpec typeSpec;
    Util::JsonArray* annotations;

    static std::optional<PortMetadata> fromTNA(
        const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::PortMetadata portMetadata;
        if (!externInstance.info().UnpackTo(&portMetadata)) {
            error("Extern instance %1% does not pack a PortMetadata object", pre.name());
            return std::nullopt;
        }
        return PortMetadata{pre.id(), pre.name(),
                            portMetadata.key_name(), portMetadata.type_spec(),
                            transformAnnotations(pre)};
    }
};

struct BFRuntimeSchemaGenerator::DynHash {
    const std::string getAlgorithmTableName() const {
        return name + ".algorithm"; }
    const std::string getConfigTableName() const {
        return name + ".configure"; }
    const std::string getComputeTableName() const {
        return name + ".compute"; }

    const cstring name;
    const P4Id cfgId;
    const P4Id cmpId;
    const P4Id algId;
    const p4configv1::P4DataTypeSpec typeSpec;
    struct hashField {
        cstring hashFieldName;        // Field Name
        bool isConstant;              // true if field is a constant
    };
    const std::vector<hashField> hashFieldInfo;
    const int hashWidth;
    Util::JsonArray* annotations;

    std::vector<cstring> getHashFieldNames() const {
        std::vector<cstring> hashFieldNames;
        for (auto &f : hashFieldInfo) {
            hashFieldNames.push_back(f.hashFieldName);
        }
        return hashFieldNames;
    }

    bool is_constant(cstring name) const {
        for (auto &f : hashFieldInfo)
            if (f.hashFieldName == name) return f.isConstant;
        return false;
    }

    static std::optional<DynHash> fromTNA(
        const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::DynHash dynHash;
        if (!externInstance.info().UnpackTo(&dynHash)) {
            error("Extern instance %1% does not pack a PortMetadata object", pre.name());
            return std::nullopt;
        }
        std::vector<hashField> hfInfo;
        for (auto f : dynHash.field_infos()) {
            hfInfo.push_back({f.field_name(), f.is_constant()});
        }
        auto cfgId = makeBFRuntimeId(pre.id(), ::barefoot::P4Ids::HASH_CONFIGURE);
        auto cmpId = makeBFRuntimeId(pre.id(), ::barefoot::P4Ids::HASH_COMPUTE);
        auto algId = makeBFRuntimeId(pre.id(), ::barefoot::P4Ids::HASH_ALGORITHM);
        return DynHash{pre.name(), cfgId, cmpId, algId, dynHash.type_spec(),
                       hfInfo, dynHash.hash_width(), transformAnnotations(pre)};
    }
};

struct BFRuntimeSchemaGenerator::Snapshot {
    std::string name;
    std::string gress;
    P4Id id;
    struct Field {
        P4Id id;
        std::string name;
        int32_t bitwidth;
    };
    std::vector<Field> fields;

    std::string getCfgTblName() const { return name + ".cfg"; }

    std::string getTrigTblName() const { return name + "." + gress + "_trigger"; }

    std::string getDataTblName() const { return name + "." + gress + "_data"; }

    std::string getLivTblName() const { return name + "." + gress + "_liveness"; }

    static std::optional<Snapshot> fromTNA(
        const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::Snapshot snapshot;
        if (!externInstance.info().UnpackTo(&snapshot)) {
            error("Extern instance %1% does not pack a Snapshot object", pre.name());
            return std::nullopt;
        }
        std::string name = snapshot.pipe() + ".snapshot";
        std::string gress;
        if (snapshot.direction() == ::barefoot::DIRECTION_INGRESS)
          gress = "ingress";
        else if (snapshot.direction() == ::barefoot::DIRECTION_EGRESS)
          gress = "egress";
        else
          BUG("Unknown direction");
        std::vector<Field> fields;
        for (const auto& f : snapshot.fields())
          fields.push_back({f.id(), f.name(), f.bitwidth()});
        return Snapshot{name, gress, pre.id(), std::move(fields)};
    }
};

struct BFRuntimeSchemaGenerator::ParserChoices {
    std::string name;
    P4Id id;
    std::vector<cstring> choices;

    static std::optional<ParserChoices> fromTNA(
        const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::ParserChoices parserChoices;
        if (!externInstance.info().UnpackTo(&parserChoices)) {
            error("Extern instance %1% does not pack a ParserChoices object", pre.name());
            return std::nullopt;
        }
        std::string name;
        // The name is "<pipe>.<gress>.$PARSER_CONFIGURE".
        if (parserChoices.direction() == ::barefoot::DIRECTION_INGRESS)
          name = parserChoices.pipe() + "." + "ingress_parser" + "." + "$PARSER_CONFIGURE";
        else if (parserChoices.direction() == ::barefoot::DIRECTION_EGRESS)
          name = parserChoices.pipe() + "." + "egress_parser" + "." + "$PARSER_CONFIGURE";
        else
          BUG("Unknown direction");
        std::vector<cstring> choices;
        // For each possible parser, we have to use the "architecture name" as
        // the name exposed to the control plane, because this is the only name
        // which is guaranteed to exist ane be unique.
        for (const auto& choice : parserChoices.choices())
          choices.push_back(choice.arch_name());
        return ParserChoices{name, pre.id(), std::move(choices)};
    }
};

struct BFRuntimeSchemaGenerator::Wred {
    std::string name;
    P4Id id;
    int64_t size;
    Util::JsonArray* annotations;

    static std::optional<Wred> fromTNA(
        const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::Wred wred;
        if (!externInstance.info().UnpackTo(&wred)) {
            error("Extern instance %1% does not pack a Wred object", pre.name());
            return std::nullopt;
        }
        return Wred{pre.name(), pre.id(), wred.size(), transformAnnotations(pre)};
    }

    static std::optional<Wred> fromTNADirect(
        const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::DirectWred wred;
        if (!externInstance.info().UnpackTo(&wred)) {
            error("Extern instance %1% does not pack a Wred object", pre.name());
            return std::nullopt;
        }
        return Wred{pre.name(), pre.id(), 0, transformAnnotations(pre)};
    }
};

std::optional<BFRuntimeSchemaGenerator::Lpf>
BFRuntimeSchemaGenerator::getDirectLpf(P4Id lpfId) const {
    if (isOfType(lpfId, ::barefoot::P4Ids::DIRECT_LPF)) {
        auto* externInstance = findExternInstance(p4info, lpfId);
        if (externInstance == nullptr) return std::nullopt;
        return Lpf::fromTNADirect(*externInstance);
    }
    return std::nullopt;
}

std::optional<BFRuntimeSchemaGenerator::Wred>
BFRuntimeSchemaGenerator::getDirectWred(P4Id wredId) const {
    if (isOfType(wredId, ::barefoot::P4Ids::DIRECT_WRED)) {
        auto* externInstance = findExternInstance(p4info, wredId);
        if (externInstance == nullptr) return std::nullopt;
        return Wred::fromTNADirect(*externInstance);
    }
    return std::nullopt;
}

std::optional<BFRuntimeSchemaGenerator::Counter>
BFRuntimeSchemaGenerator::getDirectCounter(P4Id counterId) const {
    if (isOfType(counterId, p4configv1::P4Ids::DIRECT_COUNTER)) {
        auto* counter = Standard::findDirectCounter(p4info, counterId);
        if (counter == nullptr) return std::nullopt;
        return Counter::fromDirect(*counter);
    } else if (isOfType(counterId, ::barefoot::P4Ids::DIRECT_COUNTER)) {
        auto* externInstance = findExternInstance(p4info, counterId);
        if (externInstance == nullptr) return std::nullopt;
        return fromTNADirectCounter(*externInstance);
    }
    return std::nullopt;
}

std::optional<BFRuntimeSchemaGenerator::Meter>
BFRuntimeSchemaGenerator::getDirectMeter(P4Id meterId) const {
    if (isOfType(meterId, p4configv1::P4Ids::DIRECT_METER)) {
        auto* meter = Standard::findDirectMeter(p4info, meterId);
        if (meter == nullptr) return std::nullopt;
        return Meter::fromDirect(*meter);
    } else if (isOfType(meterId, ::barefoot::P4Ids::DIRECT_METER)) {
        auto* externInstance = findExternInstance(p4info, meterId);
        if (externInstance == nullptr) return std::nullopt;
        return fromTNADirectMeter(*externInstance);
    }
    return std::nullopt;
}

std::optional<BFRuntimeSchemaGenerator::Register>
BFRuntimeSchemaGenerator::getDirectRegister(P4Id registerId) const {
    if (!isOfType(registerId, ::barefoot::P4Ids::DIRECT_REGISTER)) return std::nullopt;
    auto* externInstance = findExternInstance(p4info, registerId);
    if (externInstance == nullptr) return std::nullopt;
    return fromTNADirectRegister(*externInstance);
}

void
BFRuntimeSchemaGenerator::addActionSelectorGetMemberCommon(Util::JsonArray* tablesJson,
                                        const ActionSelector& actionSelector) const {
        auto* tableJson = initTableJson(actionSelector.get_mem_name,
                actionSelector.get_mem_id, "SelectorGetMember"_cs, 1 /* size */,
                actionSelector.annotations);

        auto* keyJson = new Util::JsonArray();
        addKeyField(keyJson, BF_RT_DATA_SELECTOR_GROUP_ID, "$SELECTOR_GROUP_ID"_cs,
                            true /* mandatory */, "Exact"_cs, makeTypeInt("uint64"_cs));
        addKeyField(keyJson, BF_RT_DATA_HASH_VALUE, "hash_value"_cs,
                            true /* mandatory */, "Exact"_cs, makeTypeInt("uint64"_cs));
        tableJson->emplace("key"_cs, keyJson);

        auto* dataJson = new Util::JsonArray();
        {
            auto* f = makeCommonDataField(BF_RT_DATA_ACTION_MEMBER_ID, "$ACTION_MEMBER_ID"_cs,
            makeTypeInt("uint64"_cs), false /* repeated */);
            addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
        }
        tableJson->emplace("data"_cs, dataJson);

        tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
        tableJson->emplace("attributes"_cs, new Util::JsonArray());
        addToDependsOn(tableJson, actionSelector.id);

        tablesJson->append(tableJson);
}

void
BFRuntimeSchemaGenerator::addActionSelectorCommon(Util::JsonArray* tablesJson,
                const ActionSelector& actionSelector) const {
    // formalize ID allocation for selector tables
    // repeat same annotations as for action table
    // the maximum number of groups is the table size for the selector table
    auto* tableJson = initTableJson(
        actionSelector.name, actionSelector.id, "Selector"_cs,
        actionSelector.num_groups, actionSelector.annotations);

    auto* keyJson = new Util::JsonArray();
    addKeyField(keyJson, TD_DATA_SELECTOR_GROUP_ID, "$SELECTOR_GROUP_ID"_cs,
            true /* mandatory */, "Exact"_cs, makeTypeInt("uint32"_cs));
    tableJson->emplace("key"_cs, keyJson);

    auto* dataJson = new Util::JsonArray();
    {
        auto* f = makeCommonDataField(
                TD_DATA_ACTION_MEMBER_ID, "$ACTION_MEMBER_ID"_cs,
                makeTypeInt("uint32"_cs), true /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
    {
        auto* f = makeCommonDataField(
                BF_RT_DATA_ACTION_MEMBER_STATUS, "$ACTION_MEMBER_STATUS"_cs,
                makeTypeBool(), true /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
    {
        auto* f = makeCommonDataField(
                BF_RT_DATA_MAX_GROUP_SIZE, "$MAX_GROUP_SIZE"_cs,
                makeTypeInt("uint32"_cs, actionSelector.max_group_size), false /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
    {
        auto* f = makeCommonDataField(
                BF_RT_DATA_ADT_OFFSET, "$ADT_OFFSET"_cs,
                makeTypeInt("uint32"_cs, actionSelector.adt_offset), false /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }

    tableJson->emplace("data"_cs, dataJson);

    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
    tableJson->emplace("attributes"_cs, new Util::JsonArray());
    addToDependsOn(tableJson, actionSelector.action_profile_id);

    tablesJson->append(tableJson);
    LOG1("Added Action Selector " << actionSelector.name);
}

bool
BFRuntimeSchemaGenerator::addActionProfIds(const p4configv1::Table& table,
        Util::JsonObject* tableJson) const {
    auto implementationId = table.implementation_id();
    auto actProfId = static_cast<P4Id>(0);
    auto actSelectorId = static_cast<P4Id>(0);
    if (implementationId > 0) {
        auto hasSelector = actProfHasSelector(implementationId);
        if (hasSelector == std::nullopt) {
            error("Invalid implementation id in p4info: %1%", implementationId);
            return false;
        }
        cstring tableType = ""_cs;
        if (*hasSelector) {
            actSelectorId = makeActSelectorId(implementationId);
            tableType = "MatchAction_Indirect_Selector"_cs;
            // actProfId will be set while visiting action profile externs
        } else {
            actProfId = makeActProfId(implementationId);
            tableType = "MatchAction_Indirect"_cs;
        }
        tableJson->erase("table_type"_cs);
        tableJson->emplace("table_type"_cs, tableType);
    }

    if (actProfId > 0) addToDependsOn(tableJson, actProfId);
    if (actSelectorId > 0) addToDependsOn(tableJson, actSelectorId);
    return true;
}

void
BFRuntimeSchemaGenerator::addActionProfs(Util::JsonArray* tablesJson) const {
    for (const auto& actionProf : p4info.action_profiles()) {
        auto actionProfInstance = ActionProf::from(p4info, actionProf);
        if (actionProfInstance == std::nullopt) continue;
        addActionProfCommon(tablesJson, *actionProfInstance);

        auto actionSelectorInstance = ActionSelector::from(p4info, actionProf);
        if (actionSelectorInstance == std::nullopt) continue;
        addActionSelectorCommon(tablesJson, *actionSelectorInstance);
    }
}

void
BFRuntimeSchemaGenerator::addValueSet(Util::JsonArray* tablesJson,
                                 const ValueSet& valueSet) const {
    auto* tableJson = initTableJson(
        valueSet.name, valueSet.id, "ParserValueSet"_cs, valueSet.size, valueSet.annotations);

    auto* keyJson = new Util::JsonArray();
    auto parser = TypeSpecParser::make(p4info, valueSet.typeSpec, "ValueSet"_cs, valueSet.name);
    for (const auto &f : parser) {
        // Make key fields not mandatory, this allows user to use a
        // driver initialized default value (0).
        addKeyField(keyJson, f.id, f.name, false /* mandatory */, "Ternary"_cs, f.type);
    }
    tableJson->emplace("key"_cs, keyJson);

    tableJson->emplace("data"_cs, new Util::JsonArray());
    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
    auto* attributesJson =  new Util::JsonArray();
    attributesJson->append("EntryScope"_cs);
    tableJson->emplace("attributes"_cs, attributesJson);

    tablesJson->append(tableJson);
}

void BFRuntimeSchemaGenerator::addDirectResources(const p4configv1::Table& table,
        Util::JsonArray* dataJson, Util::JsonArray* operationsJson,
        Util::JsonArray* attributesJson, P4Id maxActionParamId) const {
    // direct resources
    for (auto directResId : table.direct_resource_ids()) {
        if (auto counter = getDirectCounter(directResId)) {
            addCounterDataFields(dataJson, *counter);
            operationsJson->append("SyncCounters"_cs);
        } else if (auto meter = getDirectMeter(directResId)) {
            addMeterDataFields(dataJson, *meter);
            attributesJson->append("MeterByteCountAdjust"_cs);
        } else if (auto register_ = getDirectRegister(directResId)) {
            addRegisterDataFields(dataJson, *register_, maxActionParamId);
            operationsJson->append("SyncRegisters"_cs);
        } else if (auto lpf = getDirectLpf(directResId)) {
            addLpfDataFields(dataJson);
        } else if (auto lpf = getDirectWred(directResId)) {
            addWredDataFields(dataJson);
        } else {
            error("Unknown direct resource id '%1%'", directResId);
            continue;
        }
    }
}

std::optional<bool>
BFRuntimeSchemaGenerator::actProfHasSelector(P4Id actProfId) const {
    if (isOfType(actProfId, p4configv1::P4Ids::ACTION_PROFILE)) {
        auto* actionProf = Standard::findActionProf(p4info, actProfId);
        if (actionProf == nullptr) return std::nullopt;
        return actionProf->with_selector();
    } else if (isOfType(actProfId, ::barefoot::P4Ids::ACTION_PROFILE)) {
        return false;
    } else if (isOfType(actProfId, ::barefoot::P4Ids::ACTION_SELECTOR)) {
        return true;
    }
    return std::nullopt;
}

const Util::JsonObject*
BFRuntimeSchemaGenerator::genSchema() const {
    Log::TempIndent indent;
    LOG1("Generating BFRT schema" << indent);
    auto* json = new Util::JsonObject();

    json->emplace("schema_version"_cs, cstring("1.0.0"));

    auto* tablesJson = new Util::JsonArray();
    json->emplace("tables"_cs, tablesJson);

    addMatchTables(tablesJson);
    addActionProfs(tablesJson);
    addCounters(tablesJson);
    addMeters(tablesJson);
    // handle "standard" (v1model / PSA) registers

    auto* learnFiltersJson = new Util::JsonArray();
    json->emplace("learn_filters"_cs, learnFiltersJson);
    addLearnFilters(learnFiltersJson);

    addTNAExterns(tablesJson, learnFiltersJson);
    addPortMetadataExtern(tablesJson);
    addDebugCounterTable(tablesJson);

    return json;
}

void
BFRuntimeSchemaGenerator::addTNAExterns(Util::JsonArray* tablesJson,
                                      Util::JsonArray* learnFiltersJson) const {
    Log::TempIndent indent;
    LOG1("Adding TNA Externs" << indent);
    for (const auto& externType : p4info.externs()) {
        auto externTypeId = static_cast<::barefoot::P4Ids::Prefix>(externType.extern_type_id());
        if (externTypeId == ::barefoot::P4Ids::ACTION_PROFILE) {
            for (const auto& externInstance : externType.instances()) {
                auto actionProf = fromTNAActionProfile(p4info, externInstance);
                if (actionProf != std::nullopt) addActionProfCommon(tablesJson, *actionProf);
            }
        } else if (externTypeId == ::barefoot::P4Ids::ACTION_SELECTOR) {
            for (const auto& externInstance : externType.instances()) {
                auto actionSelector =
                    ActionSelector::fromTNA(p4info, externInstance);
                if (actionSelector != std::nullopt) {
                    addActionSelectorCommon(tablesJson, *actionSelector);
                    addActionSelectorGetMemberCommon(tablesJson, *actionSelector);
                }
            }
        } else if (externTypeId == ::barefoot::P4Ids::COUNTER) {
            for (const auto& externInstance : externType.instances()) {
                auto counter = fromTNACounter(externInstance);
                if (counter != std::nullopt) addCounterCommon(tablesJson, *counter);
            }
        } else if (externTypeId == ::barefoot::P4Ids::METER) {
            for (const auto& externInstance : externType.instances()) {
                auto meter = fromTNAMeter(externInstance);
                if (meter != std::nullopt) addMeterCommon(tablesJson, *meter);
            }
        } else if (externTypeId == ::barefoot::P4Ids::DIGEST) {
            for (const auto& externInstance : externType.instances()) {
                auto digest = fromTNADigest(externInstance);
                if (digest != std::nullopt) addLearnFilterCommon(learnFiltersJson, *digest);
            }
        } else if (externTypeId == ::barefoot::P4Ids::REGISTER) {
            for (const auto& externInstance : externType.instances()) {
                auto register_ = fromTNARegister(externInstance);
                if (register_ != std::nullopt)
                    addRegisterCommon(tablesJson, *register_);
            }
        } else if (externTypeId == ::barefoot::P4Ids::REGISTER_PARAM) {
            for (const auto& externInstance : externType.instances()) {
                auto register_param_ = RegisterParam::fromTNA(externInstance);
                if (register_param_ != std::nullopt)
                    addRegisterParam(tablesJson, *register_param_);
            }
        } else if (externTypeId == ::barefoot::P4Ids::LPF) {
            for (const auto& externInstance : externType.instances()) {
                auto lpf = Lpf::fromTNA(externInstance);
                if (lpf != std::nullopt) addLpf(tablesJson, *lpf);
            }
        } else if (externTypeId == ::barefoot::P4Ids::WRED) {
            for (const auto& externInstance : externType.instances()) {
                auto wred = Wred::fromTNA(externInstance);
                if (wred != std::nullopt) addWred(tablesJson, *wred);
            }
        } else if (externTypeId == ::barefoot::P4Ids::VALUE_SET) {
            for (const auto& externInstance : externType.instances()) {
                auto valueSet = ValueSet::fromTNA(externInstance);
                if (valueSet != std::nullopt) addValueSet(tablesJson, *valueSet);
            }
        } else if (externTypeId == ::barefoot::P4Ids::SNAPSHOT) {
            for (const auto& externInstance : externType.instances()) {
                auto snapshot = Snapshot::fromTNA(externInstance);
                if (snapshot != std::nullopt) {
                    addSnapshot(tablesJson, *snapshot);
                    addSnapshotLiveness(tablesJson, *snapshot);
                }
            }
        } else if (externTypeId == ::barefoot::P4Ids::HASH) {
            for (const auto& externInstance : externType.instances()) {
                auto dynHash = DynHash::fromTNA(externInstance);
                if (dynHash != std::nullopt) {
                    addDynHash(tablesJson, *dynHash);
                }
            }
        } else if (externTypeId == ::barefoot::P4Ids::PARSER_CHOICES) {
            for (const auto& externInstance : externType.instances()) {
                auto parserChoices = ParserChoices::fromTNA(externInstance);
                if (parserChoices != std::nullopt) {
                    // Disabled unless driver adds BF-RT support for dynamically
                    // changing parser configurations
                    // addParserChoices(tablesJson, *parserChoices);
                }
            }
        }
    }
}

// I chose to do this in a separate function rather than in addTNAExterns
// because if there is no PortMetadata extern in P4Info, we need to generate a
// default one.
void
BFRuntimeSchemaGenerator::addPortMetadataExtern(Util::JsonArray* tablesJson) const {
    Log::TempIndent indent;
    LOG1("Adding PortMetadata Extern" << indent);
    for (const auto& externType : p4info.externs()) {
        auto externTypeId = static_cast<::barefoot::P4Ids::Prefix>(externType.extern_type_id());
        if (externTypeId == ::barefoot::P4Ids::PORT_METADATA) {
            // For multipipe and multiple ingress per pipe scenarios we can have
            // multiple port metadata extern instances in the program, here we
            // add all instances collected in the program
            for (const auto& externInstance : externType.instances()) {
                auto portMetadata = PortMetadata::fromTNA(externInstance);
                if (portMetadata != std::nullopt) addPortMetadata(tablesJson, *portMetadata);
            }
        }
    }
}

void
BFRuntimeSchemaGenerator::addRegisterParamDataFields(Util::JsonArray* dataJson,
                                                const RegisterParam& register_param_,
                                                P4Id idOffset) const {
    auto typeSpec = register_param_.typeSpec;
    Util::JsonObject *type = nullptr;
    if (typeSpec.has_bitstring()) {
        if (typeSpec.bitstring().has_bit()) {
            type = makeTypeBytes(typeSpec.bitstring().bit().bitwidth(),
                register_param_.initial_value);
        } else if (typeSpec.bitstring().has_int_()) {
            type = makeTypeBytes(typeSpec.bitstring().int_().bitwidth(),
                register_param_.initial_value);
        }
    }
    if (type == nullptr) return;
    auto *f = makeCommonDataField(idOffset, "value"_cs, type, false /* repeated */);
    addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
}

void
BFRuntimeSchemaGenerator::addRegisterParam(Util::JsonArray* tablesJson,
                                      const RegisterParam& register_param_) const {
    auto* tableJson = initTableJson(register_param_.name, register_param_.id,
        "RegisterParam"_cs, 1 /* size */, register_param_.annotations);

    // The register or M/A table the register parameter is attached to
    // The zero value means it is not used anywhere and hasn't been optimized out.
    if (register_param_.tableId != 0)
        addToDependsOn(tableJson, register_param_.tableId);

    tableJson->emplace("key"_cs, new Util::JsonArray());

    auto* dataJson = new Util::JsonArray();
    addRegisterParamDataFields(dataJson, register_param_);
    tableJson->emplace("data"_cs, dataJson);

    tableJson->emplace("attributes"_cs, new Util::JsonArray());
    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());

    tablesJson->append(tableJson);
    LOG2("Added Register Param " << register_param_.name);
}

void
BFRuntimeSchemaGenerator::addPortMetadata(Util::JsonArray* tablesJson,
                                     const PortMetadata& portMetadata) const {
    auto* tableJson = initTableJson(
        portMetadata.name, portMetadata.id, "PortMetadata"_cs, Device::numMaxChannels(),
        portMetadata.annotations);

    auto* keyJson = new Util::JsonArray();
    addKeyField(keyJson, BF_RT_DATA_PORT_METADATA_PORT, portMetadata.key_name + ".ingress_port",
                true /* mandatory */, "Exact"_cs, makeTypeBytes(Device::portBitWidth()));
    tableJson->emplace("key"_cs, keyJson);

    auto* dataJson = new Util::JsonArray();
    if (portMetadata.typeSpec.type_spec_case() !=
        ::p4::config::v1::P4DataTypeSpec::TYPE_SPEC_NOT_SET) {
        auto* fieldsJson = new Util::JsonArray();
        transformTypeSpecToDataFields(
            fieldsJson, portMetadata.typeSpec, "PortMetadata"_cs,
            portMetadata.name);
        for (auto* f : *fieldsJson) {
            addSingleton(dataJson, f->to<Util::JsonObject>(),
                         true /* mandatory */, false /* read-only */);
      }
    } else {
        auto* f = makeCommonDataField(
            BF_RT_DATA_PORT_METADATA_DEFAULT_FIELD, "$DEFAULT_FIELD"_cs,
            makeTypeBytes(Device::pardeSpec().bitPhase0Size()), false /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
    tableJson->emplace("data"_cs, dataJson);

    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
    tableJson->emplace("attributes"_cs, new Util::JsonArray());

    tablesJson->append(tableJson);
    LOG2("Added PortMetadata " << portMetadata.name);
}

void
BFRuntimeSchemaGenerator::addDynHash(Util::JsonArray* tablesJson,
                                     const DynHash& dynHash) const {
    addDynHashAlgorithm(tablesJson, dynHash);
    addDynHashConfig(tablesJson, dynHash);
    addDynHashCompute(tablesJson, dynHash);
}

void
BFRuntimeSchemaGenerator::addDynHashConfig(Util::JsonArray* tablesJson,
                                     const DynHash& dynHash) const {
    // Add <hash>.configure Table
    auto* tableJson = initTableJson(dynHash.getConfigTableName(), dynHash.cfgId,
            "DynHashConfigure"_cs, 1 /* size */, dynHash.annotations);

    tableJson->emplace("key"_cs, new Util::JsonArray());  // empty key for configure table

    auto* dataJson = new Util::JsonArray();
    auto hashFieldNames = dynHash.getHashFieldNames();
    auto parser = TypeSpecParser::make(
        p4info, dynHash.typeSpec, "DynHash"_cs, dynHash.name, &hashFieldNames, ""_cs, ""_cs);
    int numConstants = 0;
    for (const auto &field : parser) {
        auto* containerItemsJson = new Util::JsonArray();
        auto fLength = field.type->get("width"_cs)->to<Util::JsonValue>()->getInt();
        {
            auto* f = makeCommonDataField(BF_RT_DATA_HASH_CONFIGURE_START_BIT, "start_bit"_cs,
                makeTypeInt("uint64"_cs, 0), false /* repeated */);
            addSingleton(containerItemsJson, f, false /* mandatory */, false /* read-only */);
        }
        {
            auto* f = makeCommonDataField(BF_RT_DATA_HASH_CONFIGURE_LENGTH, "length"_cs,
                makeTypeInt("uint64"_cs, fLength), false /* repeated */);
            addSingleton(containerItemsJson, f, false /* mandatory */, false /* read-only */);
        }
        {
            auto* f = makeCommonDataField(BF_RT_DATA_HASH_CONFIGURE_ORDER, "order"_cs,
                makeTypeInt("uint64"_cs), false /* repeated */);
            addSingleton(containerItemsJson, f, true /* mandatory */, false /* read-only */);
        }
        cstring field_prefix = ""_cs;
        Util::JsonArray *annotations = nullptr;
        if (dynHash.is_constant(field.name)) {
            // Constant fields have unique field names with the format
            // constant<id>_<size>_<value>
            // id increments by 1 for every constant field.
            // Should match names generated in context.json (mau/dynhash.cpp)
            field_prefix = "constant" + std::to_string(numConstants++)
                            + "_" + std::to_string(fLength) + "_";
            auto *constantAnnot = new Util::JsonObject();
            constantAnnot->emplace("name"_cs, "bfrt_p4_constant"_cs);
            annotations = new Util::JsonArray();
            annotations->append(constantAnnot);
        }
        auto* fJson = makeContainerDataField(field.id, field_prefix + field.name,
                containerItemsJson, true /* repeated */, annotations);
        addSingleton(dataJson, fJson, false /* mandatory */, false /* read-only */);
    }
    tableJson->emplace("data"_cs, dataJson);
    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
    tableJson->emplace("attributes"_cs, new Util::JsonArray());

    tablesJson->append(tableJson);
    LOG2("Added DynHashConfigure " << dynHash.getConfigTableName());
}

void
BFRuntimeSchemaGenerator::addDynHashAlgorithm(Util::JsonArray* tablesJson,
                                     const DynHash& dynHash) const {
    auto* tableJson = initTableJson(
        dynHash.getAlgorithmTableName(), dynHash.algId, "DynHashAlgorithm"_cs,
        1 /* size */, dynHash.annotations);

    tableJson->emplace("key"_cs, new Util::JsonArray());  // empty key

    auto* dataJson = new Util::JsonArray();
    // Add rotate key field
    auto* rotateJson = makeCommonDataField(BF_RT_DATA_HASH_ALGORITHM_ROTATE, "rotate"_cs,
                makeTypeInt("uint32"_cs, 0 /* default */), false /* repeated */);
    addSingleton(dataJson, rotateJson, false /* mandatory */, false /* read-only */);

    // Add seed key field
    auto* seedJson = makeCommonDataField(BF_RT_DATA_HASH_ALGORITHM_SEED, "seed"_cs,
                makeTypeInt("uint32"_cs, 0 /* default */), false /* repeated */);
    addSingleton(dataJson, seedJson, false /* mandatory */, false /* read-only */);

    // Add msb key field
    auto* msbJson = makeCommonDataField(BF_RT_DATA_HASH_ALGORITHM_MSB, "msb"_cs,
            makeTypeBool(false /* default */), false /* repeated */);
    addSingleton(dataJson, msbJson, false /* mandatory */, false /* read-only */);

    // Add extend key field
    auto* extendJson = makeCommonDataField(BF_RT_DATA_HASH_ALGORITHM_EXTEND, "extend"_cs,
            makeTypeBool(false /* default */), false /* repeated */);
    addSingleton(dataJson, extendJson, false /* mandatory */, false /* read-only */);

    tableJson->emplace("data"_cs, dataJson);

    auto* actionSpecsJson = new Util::JsonArray();

    // Add pre-defined action spec
    auto* preDefinedJson = new Util::JsonObject();
    preDefinedJson->emplace("id"_cs, BF_RT_DATA_HASH_ALGORITHM_PRE_DEFINED);
    preDefinedJson->emplace("name"_cs, "pre_defined"_cs);
    preDefinedJson->emplace("action_scope"_cs, "TableAndDefault"_cs);
    preDefinedJson->emplace("annotations"_cs, new Util::JsonArray());

    P4Id dataId = 1;
    auto* preDefinedDataJson = new Util::JsonArray();
    // Add algorithm_name field
    std::vector<cstring> algos;
    // Create a list of all supported algos from bf-utils repo
    for (int i = 0; i < static_cast<int>(INVALID_DYN); i++) {
        char alg_name[BF_UTILS_ALGO_NAME_LEN];
        bfn_hash_alg_type_t alg_t = static_cast<bfn_hash_alg_type_t>(i);
        if (alg_t == CRC_DYN) continue;  // skip CRC as we include its sub types later
        hash_alg_type_to_str(alg_t, alg_name);
        algos.emplace_back(alg_name);
    }
    // Include all CRC Sub Types
    for (int i = 0; i < static_cast<int>(CRC_INVALID); i++) {
        char crc_name[BF_UTILS_ALGO_NAME_LEN];
        bfn_crc_alg_t alg_t = static_cast<bfn_crc_alg_t>(i);
        crc_alg_type_to_str(alg_t, crc_name);
        algos.emplace_back(crc_name);
    }
    auto* algoType = makeTypeEnum(algos, cstring("RANDOM") /* default */);
    addActionDataField(preDefinedDataJson, dataId, "algorithm_name",
            false /* mandatory */, false /* read_only */,
            algoType, nullptr /* annotations */);
    preDefinedJson->emplace("data"_cs, preDefinedDataJson);

    actionSpecsJson->append(preDefinedJson);
    addToDependsOn(tableJson, dynHash.cfgId);

    // Add user_defined action spec
    auto* userDefinedJson = new Util::JsonObject();
    userDefinedJson->emplace("id"_cs, BF_RT_DATA_HASH_ALGORITHM_USER_DEFINED);
    userDefinedJson->emplace("name"_cs, "user_defined"_cs);
    userDefinedJson->emplace("action_scope"_cs, "TableAndDefault"_cs);
    userDefinedJson->emplace("annotations"_cs, new Util::JsonArray());

    auto* userDefinedDataJson = new Util::JsonArray();
    dataId = 1;
    // Add reverse data field
    auto* revType = makeTypeBool(false /* default */);
    addActionDataField(userDefinedDataJson, dataId++, "reverse",
            false /* mandatory */, false /* read_only */,
            revType, nullptr /* annotations */);

    // Add polynomial data field
    auto* polyType = makeTypeInt("uint64"_cs);
    addActionDataField(userDefinedDataJson, dataId++, "polynomial",
            true /* mandatory */, false /* read_only */,
            polyType, nullptr /* annotations */);

    // Add init data field
    auto* initType = makeTypeInt("uint64"_cs, 0 /* default_value */);
    addActionDataField(userDefinedDataJson, dataId++, "init",
            false /* mandatory */, false /* read_only */,
            initType, nullptr /* annotations */);

    // Add final_xor data field
    auto* finalXorType = makeTypeInt("uint64"_cs, 0 /* default_value */);
    addActionDataField(userDefinedDataJson, dataId++, "final_xor",
            false /* mandatory */, false /* read_only */,
            finalXorType, nullptr /* annotations */);

    // Add hash_bit_width data field
    auto* hashBitWidthType = makeTypeInt("uint64"_cs);
    addActionDataField(userDefinedDataJson, dataId++, "hash_bit_width",
            false /* mandatory */, false /* read_only */,
            hashBitWidthType, nullptr /* annotations */);
    userDefinedJson->emplace("data"_cs, userDefinedDataJson);

    actionSpecsJson->append(userDefinedJson);

    tableJson->emplace("action_specs"_cs, actionSpecsJson);
    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
    tableJson->emplace("attributes"_cs, new Util::JsonArray());

    tablesJson->append(tableJson);
    LOG2("Added DynHashAlgorithm " << dynHash.getAlgorithmTableName());
}

void
BFRuntimeSchemaGenerator::addDynHashCompute(Util::JsonArray* tablesJson,
                                     const DynHash& dynHash) const {
    // Add <hash>.compute Table
    auto* tableJson = initTableJson(
        dynHash.getComputeTableName(), dynHash.cmpId, "DynHashCompute"_cs,
        1 /* size */, dynHash.annotations);

    P4Id id = 1;
    auto* keyJson = new Util::JsonArray();
    auto hashFieldNames = dynHash.getHashFieldNames();
    auto parser = TypeSpecParser::make(
        p4info, dynHash.typeSpec, "DynHash"_cs, dynHash.name, &hashFieldNames, ""_cs, ""_cs);
    for (const auto &field : parser) {
        if (dynHash.is_constant(field.name)) {
            // If this is a constant field, then skip adding a field
            // in compute table
            continue;
        }
        auto* type = makeTypeBytes(field.type->get("width"_cs)->to<Util::JsonValue>()->getInt());
        addKeyField(keyJson, id++, field.name, false /* mandatory */, "Exact"_cs, type);
    }
    tableJson->emplace("key"_cs, keyJson);

    auto* dataJson = new Util::JsonArray();
    auto* f = makeCommonDataField(
        BF_RT_DATA_HASH_VALUE, "hash_value"_cs,
        makeTypeBytes(dynHash.hashWidth), false /* repeated */);
    addSingleton(dataJson, f, false /* mandatory */, true /* read-only */);
    tableJson->emplace("data"_cs, dataJson);

    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
    tableJson->emplace("attributes"_cs, new Util::JsonArray());
    tableJson->emplace("read_only"_cs, true);
    addToDependsOn(tableJson, dynHash.cfgId);

    tablesJson->append(tableJson);
    LOG2("Added DynHashCompute " << dynHash.getComputeTableName());
}

void
BFRuntimeSchemaGenerator::addLpfDataFields(Util::JsonArray* dataJson) {
    {
        auto* f = makeCommonDataField(
            BF_RT_DATA_LPF_SPEC_TYPE, "$LPF_SPEC_TYPE"_cs,
            makeTypeEnum({"RATE"_cs, "SAMPLE"_cs}), false /* repeated */);
        addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
    }
    {
        auto* f = makeCommonDataField(
            BF_RT_DATA_LPF_SPEC_GAIN_TIME_CONSTANT_NS, "$LPF_SPEC_GAIN_TIME_CONSTANT_NS"_cs,
            makeTypeFloat("float"_cs), false /* repeated */);
        addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
    }
    {
        auto* f = makeCommonDataField(
            BF_RT_DATA_LPF_SPEC_DECAY_TIME_CONSTANT_NS, "$LPF_SPEC_DECAY_TIME_CONSTANT_NS"_cs,
            makeTypeFloat("float"_cs), false /* repeated */);
        addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
    }
    {
        auto* f = makeCommonDataField(
            BF_RT_DATA_LPF_SPEC_OUT_SCALE_DOWN_FACTOR, "$LPF_SPEC_OUT_SCALE_DOWN_FACTOR"_cs,
            makeTypeInt("uint32"_cs), false /* repeated */);
        addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
    }
}

void
BFRuntimeSchemaGenerator::addLpf(Util::JsonArray* tablesJson, const Lpf& lpf) const {
    auto* tableJson = initTableJson(lpf.name, lpf.id, "Lpf"_cs, lpf.size, lpf.annotations);

    auto* keyJson = new Util::JsonArray();
    addKeyField(keyJson, BF_RT_DATA_LPF_INDEX, "$LPF_INDEX"_cs,
                true /* mandatory */, "Exact"_cs, makeTypeInt("uint32"_cs));
    tableJson->emplace("key"_cs, keyJson);

    auto* dataJson = new Util::JsonArray();
    addLpfDataFields(dataJson);
    tableJson->emplace("data"_cs, dataJson);

    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
    tableJson->emplace("attributes"_cs, new Util::JsonArray());

    tablesJson->append(tableJson);
}

void
BFRuntimeSchemaGenerator::addWredDataFields(Util::JsonArray* dataJson) {
    {
        auto* f = makeCommonDataField(
            BF_RT_DATA_WRED_SPEC_TIME_CONSTANT_NS, "$WRED_SPEC_TIME_CONSTANT_NS"_cs,
            makeTypeFloat("float"_cs), false /* repeated */);
        addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
    }
    {
        auto* f = makeCommonDataField(
            BF_RT_DATA_WRED_SPEC_MIN_THRESH_CELLS, "$WRED_SPEC_MIN_THRESH_CELLS"_cs,
            makeTypeInt("uint32"_cs), false /* repeated */);
        addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
    }
    {
        auto* f = makeCommonDataField(
            BF_RT_DATA_WRED_SPEC_MAX_THRESH_CELLS, "$WRED_SPEC_MAX_THRESH_CELLS"_cs,
            makeTypeInt("uint32"_cs), false /* repeated */);
        addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
    }
    {
        auto* f = makeCommonDataField(
            BF_RT_DATA_WRED_SPEC_MAX_PROBABILITY, "$WRED_SPEC_MAX_PROBABILITY"_cs,
            makeTypeFloat("float"_cs), false /* repeated */);
        addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
    }
}

void
BFRuntimeSchemaGenerator::addWred(Util::JsonArray* tablesJson, const Wred& wred) const {
    auto* tableJson = initTableJson(wred.name, wred.id, "Wred"_cs, wred.size, wred.annotations);

    auto* keyJson = new Util::JsonArray();
    addKeyField(keyJson, BF_RT_DATA_WRED_INDEX, "$WRED_INDEX"_cs,
                true /* mandatory */, "Exact"_cs, makeTypeInt("uint32"_cs));
    tableJson->emplace("key"_cs, keyJson);

    auto* dataJson = new Util::JsonArray();
    addWredDataFields(dataJson);
    tableJson->emplace("data"_cs, dataJson);

    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
    tableJson->emplace("attributes"_cs, new Util::JsonArray());

    tablesJson->append(tableJson);
}

void
BFRuntimeSchemaGenerator::addSnapshot(Util::JsonArray* tablesJson,
                                        const Snapshot& snapshot) const {
    auto size = Device::numStages();

    // Snapshot Config Table
    {
        auto tblName = snapshot.getCfgTblName();
        Util::JsonObject *tableJson = findJsonTable(tablesJson, tblName);
        if (!tableJson) {
            auto* tableJson = initTableJson(tblName,
                    makeBFRuntimeId(snapshot.id, ::barefoot::P4Ids::SNAPSHOT),
                    "SnapshotCfg"_cs, size);

            auto* keyJson = new Util::JsonArray();
            addKeyField(keyJson, BF_RT_DATA_SNAPSHOT_START_STAGE, "start_stage"_cs,
                        false /* mandatory */, "Exact"_cs, makeTypeInt("uint32"_cs, 0));
            addKeyField(keyJson, BF_RT_DATA_SNAPSHOT_END_STAGE, "end_stage"_cs,
                        false /* mandatory */, "Exact"_cs, makeTypeInt("uint32"_cs, size - 1));
            tableJson->emplace("key"_cs, keyJson);

            auto* dataJson = new Util::JsonArray();
            {
                auto* f = makeCommonDataField(BF_RT_DATA_SNAPSHOT_TIMER_ENABLE, "timer_enable"_cs,
                        makeTypeBool(false), false /* repeated */);
                addSingleton(dataJson, f, false, false);
            }
            {
                auto* f = makeCommonDataField(BF_RT_DATA_SNAPSHOT_TIMER_VALUE_USECS,
                        "timer_value_usecs"_cs, makeTypeInt("uint32"_cs), false /* repeated */);
                addSingleton(dataJson, f, false, false);
            }
            {
                std::vector<cstring> choices = {"INGRESS"_cs, "GHOST"_cs, "INGRESS_GHOST"_cs,
                                                "ANY"_cs};
                cstring defaultChoice = "INGRESS"_cs;
                auto* f = makeCommonDataField(BF_RT_DATA_SNAPSHOT_INGRESS_TRIGGER_MODE,
                        "ingress_trigger_mode"_cs, makeTypeEnum(choices, defaultChoice),
                        false /* repeated */);
                addSingleton(dataJson, f, false, false);
            }
            {
                auto* f = makeCommonDataField(BF_RT_DATA_SNAPSHOT_THREAD, "thread"_cs,
                        makeTypeEnum({"INGRESS"_cs, "EGRESS"_cs, "ALL"_cs}), false /* repeated */);
                addSingleton(dataJson, f, true, false);
            }
            {
                auto* f = makeCommonDataField(BF_RT_DATA_SNAPSHOT_INGRESS_CAPTURE,
                        "ingress_capture"_cs, makeTypeInt("uint32"_cs), true /* repeated */);
                addROSingleton(dataJson, f);
            }
            {
                auto* f = makeCommonDataField(BF_RT_DATA_SNAPSHOT_EGRESS_CAPTURE,
                        "egress_capture"_cs, makeTypeInt("uint32"_cs), true /* repeated */);
                addROSingleton(dataJson, f);
            }
            {
                auto* f = makeCommonDataField(BF_RT_DATA_SNAPSHOT_GHOST_CAPTURE,
                        "ghost_capture"_cs, makeTypeInt("uint32"_cs), true /* repeated */);
                addROSingleton(dataJson, f);
            }
            tableJson->emplace("data"_cs, dataJson);

            tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
            tableJson->emplace("attributes"_cs, new Util::JsonArray());

            tablesJson->append(tableJson);
            LOG2("Added SnapshotCfg " << tblName);
        }
    }

    // Snapshot Trigger Table (ingress / egress)
    {
        auto tblName = snapshot.getTrigTblName();
        auto tableId = makeBFRuntimeId(snapshot.id, ::barefoot::P4Ids::SNAPSHOT_TRIGGER);
        auto* tableJson = initTableJson(tblName, tableId, "SnapshotTrigger"_cs, size);

        auto* keyJson = new Util::JsonArray();
        addKeyField(keyJson, BF_RT_DATA_SNAPSHOT_START_STAGE, "stage"_cs,
                    true /* mandatory */, "Exact"_cs, makeTypeInt("uint32"_cs, 0));
        tableJson->emplace("key"_cs, keyJson);

        auto* dataJson = new Util::JsonArray();
        {
            auto* f = makeCommonDataField(BF_RT_DATA_SNAPSHOT_ENABLE, "enable"_cs,
                    makeTypeBool(false), false /* repeated */);
            addSingleton(dataJson, f, false, false);
        }
        {
            auto *f = makeCommonDataField(
                BF_RT_DATA_SNAPSHOT_TRIGGER_STATE, "trigger_state"_cs,
                makeTypeEnum({"PASSIVE"_cs, "ARMED"_cs, "FULL"_cs}, std::nullopt),
                true /* repeated */);
            addROSingleton(dataJson, f);
        }
        for (const auto& sF : snapshot.fields) {
            {
                auto* f = makeCommonDataField(sF.id, "trig." + sF.name,
                    makeTypeBytes(sF.bitwidth, std::nullopt), false /* repeated */);
                addSingleton(dataJson, f, false, false);
            }
            {
                auto *f = makeCommonDataField(sF.id + 0x8000, "trig." + sF.name + "_mask",
                    makeTypeBytes(sF.bitwidth, std::nullopt), false /* repeated */);
                addSingleton(dataJson, f, false, false);
            }
        }
        tableJson->emplace("data"_cs, dataJson);

        tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
        tableJson->emplace("attributes"_cs, new Util::JsonArray());

        tablesJson->append(tableJson);
        LOG2("Added SnapshotTrigger " << tblName);

        // Add trigger table id to config table
        if (auto *snapCfgTable = findJsonTable(tablesJson, snapshot.getCfgTblName())) {
            addToDependsOn(snapCfgTable, tableId);
        }
    }

    // Snapshot Data Table
    {
        auto tblName = snapshot.getDataTblName();
        Util::JsonArray* dataJson = nullptr;
        auto *tableJson = initTableJson(tblName,
            makeBFRuntimeId(snapshot.id, ::barefoot::P4Ids::SNAPSHOT_DATA),
            "SnapshotData"_cs, size);

        auto* keyJson = new Util::JsonArray();
        addKeyField(keyJson, BF_RT_DATA_SNAPSHOT_START_STAGE, "stage"_cs,
                    true /* mandatory */, "Exact"_cs, makeTypeInt("uint32"_cs, 0));
        tableJson->emplace("key"_cs, keyJson);

        dataJson = new Util::JsonArray();
        {
            auto* f = makeCommonDataField(
                BF_RT_DATA_SNAPSHOT_PREV_STAGE_TRIGGER, "prev_stage_trigger"_cs,
                makeTypeBool(false), false /* repeated */);
            addROSingleton(dataJson, f);
        }
        {
            auto* f = makeCommonDataField(
                BF_RT_DATA_SNAPSHOT_TIMER_TRIGGER, "timer_trigger"_cs,
                makeTypeBool(false), false /* repeated */);
            addROSingleton(dataJson, f);
        }
        {
            auto* f = makeCommonDataField(
                BF_RT_DATA_SNAPSHOT_LOCAL_STAGE_TRIGGER, "local_stage_trigger"_cs,
                makeTypeBool(false), false /* repeated */);
            addROSingleton(dataJson, f);
        }
        {
            auto* f = makeCommonDataField(
                BF_RT_DATA_SNAPSHOT_NEXT_TABLE_NAME, "next_table_name"_cs,
                makeTypeString(), false /* repeated */);
            addROSingleton(dataJson, f);
        }

        {  // table ALU information
            auto* tableContainerItemsJson = new Util::JsonArray();
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_TABLE_NAME, "table_name"_cs,
                    makeTypeString(), false /* repeated */);
                addROSingleton(tableContainerItemsJson, f);
            }
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_MATCH_HIT_HANDLE, "match_hit_handle"_cs,
                    makeTypeInt("uint32"_cs), false /* repeated */);
                addROSingleton(tableContainerItemsJson, f);
            }
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_TABLE_HIT, "table_hit"_cs,
                    makeTypeBool(), false /* repeated */);
                addROSingleton(tableContainerItemsJson, f);
            }
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_TABLE_INHIBITED, "table_inhibited"_cs,
                    makeTypeBool(), false /* repeated */);
                addROSingleton(tableContainerItemsJson, f);
            }
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_TABLE_EXECUTED, "table_executed"_cs,
                    makeTypeBool(), false /* repeated */);
                addROSingleton(tableContainerItemsJson, f);
            }
            auto* tableContainerJson = makeContainerDataField(
                BF_RT_DATA_SNAPSHOT_TABLE_INFO, "table_info"_cs,
                tableContainerItemsJson, true /* repeated */);

            addROSingleton(dataJson, tableContainerJson);
        }
        if (Device::currentDevice() == Device::JBAY) {
            // This is likely not appropriate / sufficient for
            // MPR. Maybe this should be a repeated field of table ids / names
            // instead...
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_ENABLED_NEXT_TABLES, "enabled_next_tables"_cs,
                    makeTypeString(), true /* repeated */);
                addROSingleton(dataJson, f);
            }
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_GLOBAL_EXECUTE_TABLES, "global_execute_tables"_cs,
                    makeTypeString(), true /* repeated */);
                addROSingleton(dataJson, f);
            }
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_ENABLED_GLOBAL_EXECUTE_TABLES,
                    "enabled_global_execute_tables"_cs,
                    makeTypeString(), true /* repeated */);
                addROSingleton(dataJson, f);
            }
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_LONG_BRANCH_TABLES, "long_branch_tables"_cs,
                    makeTypeString(), true /* repeated */);
                addROSingleton(dataJson, f);
            }
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_ENABLED_LONG_BRANCH_TABLES, "enabled_long_branch_tables"_cs,
                    makeTypeString(), true /* repeated */);
                addROSingleton(dataJson, f);
            }
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_DEPARSER_ERROR, "deparser_error"_cs,
                    makeTypeBool(), false /* repeated */);
                addROSingleton(dataJson, f);
            }
            // meter ALU information
            auto* meterContainerItemsJson = new Util::JsonArray();
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_METER_TABLE_NAME, "table_name"_cs,
                    makeTypeString(), false /* repeated */);
                addROSingleton(meterContainerItemsJson, f);
            }
            {
                auto* f = makeCommonDataField(
                    BF_RT_DATA_SNAPSHOT_METER_ALU_OPERATION_TYPE,
                    "meter_alu_operation_type"_cs,
                    makeTypeString(), false /* repeated */);
                addROSingleton(meterContainerItemsJson, f);
            }
            auto* meterContainerJson = makeContainerDataField(
                BF_RT_DATA_SNAPSHOT_METER_ALU_INFO, "meter_alu_info"_cs,
                meterContainerItemsJson, true /* repeated */);
            addROSingleton(dataJson, meterContainerJson);
        }

        for (const auto& sF : snapshot.fields) {
            auto* f = makeCommonDataField(
                sF.id, sF.name, makeTypeBytes(sF.bitwidth, std::nullopt),
                false /* repeated */);
            addROSingleton(dataJson, f);
        }

        tableJson->emplace("data"_cs, dataJson);

        tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
        tableJson->emplace("attributes"_cs, new Util::JsonArray());

        tablesJson->append(tableJson);
        LOG2("Added SnapshotData " << tblName);
    }
}

void
BFRuntimeSchemaGenerator::addSnapshotLiveness(Util::JsonArray* tablesJson,
    const Snapshot& snapshot) const {
  auto tblName = snapshot.getLivTblName();
  Util::JsonObject *tableJson = findJsonTable(tablesJson, tblName);
  if (!tableJson) {
    auto tableId = makeBFRuntimeId(snapshot.id, ::barefoot::P4Ids::SNAPSHOT_LIVENESS);
    auto* tableJson = initTableJson(tblName, tableId,
        "SnapshotLiveness"_cs, 0 /* size, read-only table */);

    auto* keyJson = new Util::JsonArray();
    addKeyField(keyJson, BF_RT_DATA_SNAPSHOT_LIVENESS_FIELD_NAME, "field_name"_cs,
        true /* mandatory */, "Exact"_cs, makeTypeString());
    tableJson->emplace("key"_cs, keyJson);

    auto* dataJson = new Util::JsonArray();
    {
      auto* f = makeCommonDataField(
          BF_RT_DATA_SNAPSHOT_LIVENESS_VALID_STAGES, "valid_stages"_cs,
          makeTypeInt("uint32"_cs), true /* repeated */);
      addROSingleton(dataJson, f);
    }
    tableJson->emplace("data"_cs, dataJson);

    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
    tableJson->emplace("attributes"_cs, new Util::JsonArray());

    tablesJson->append(tableJson);
    LOG2("Added SnapshotLiveness " << tblName);
  }
}

void
BFRuntimeSchemaGenerator::addParserChoices(Util::JsonArray* tablesJson,
                                      const ParserChoices& parserChoices) const {
    auto* tableJson = initTableJson(
        parserChoices.name, parserChoices.id, "ParserInstanceConfigure"_cs,
        Device::numParsersPerPipe() /* size */);

    auto* keyJson = new Util::JsonArray();
    addKeyField(keyJson, BF_RT_DATA_PARSER_INSTANCE, "$PARSER_INSTANCE"_cs,
                true /* mandatory */, "Exact"_cs, makeTypeInt("uint32"_cs));
    tableJson->emplace("key"_cs, keyJson);

    auto* dataJson = new Util::JsonArray();
    {
        auto* f = makeCommonDataField(
            BF_RT_DATA_PARSER_NAME, "$PARSER_NAME"_cs,
            makeTypeEnum(parserChoices.choices), false /* repeated */);
        addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
    }
    tableJson->emplace("data"_cs, dataJson);

    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
    tableJson->emplace("attributes"_cs, new Util::JsonArray());

    tablesJson->append(tableJson);
    LOG2("Added ParserChoices " << parserChoices.name);
}

// Debug counter table is functionally dependent on P4 program and should
// be placed in the root of P4 program node, even if it doesn't use any
// p4info data directly.
void
BFRuntimeSchemaGenerator::addDebugCounterTable(Util::JsonArray* tablesJson) const {
    Log::TempIndent indent;
    LOG1("Adding Debug Counter Table" << indent);
    P4Id id = makeBFRuntimeId(0, barefoot::P4Ids::DEBUG_COUNTER);
    auto tblName = "tbl_dbg_counter";
    auto* tableJson = initTableJson(tblName, id, "TblDbgCnt"_cs,
        Device::numStages() * Device::numLogTablesPerStage() /* size */);

    auto* keyJson = new Util::JsonArray();
    addKeyField(keyJson, BF_RT_DATA_DEBUG_CNT_TABLE_NAME, "tbl_name"_cs,
                true /* mandatory */, "Exact"_cs, makeTypeString());
    tableJson->emplace("key"_cs, keyJson);

    auto* dataJson = new Util::JsonArray();
    {
        std::vector<cstring> choices = { "DISABLED"_cs, "TBL_MISS"_cs, "TBL_HIT"_cs,
            "GW_TBL_MISS"_cs, "GW_TBL_HIT"_cs, "GW_TBL_INHIBIT"_cs };
        auto* f = makeCommonDataField(
            BF_RT_DATA_DEBUG_CNT_TYPE, "type"_cs,
            makeTypeEnum(choices, choices[2]), false /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
    {
        auto* f = makeCommonDataField(
          BF_RT_DATA_DEBUG_CNT_VALUE, "value"_cs,
          makeTypeInt("uint64"_cs, 0), false /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
    tableJson->emplace("data"_cs, dataJson);

    tableJson->emplace("supported_operations"_cs, new Util::JsonArray());
    tableJson->emplace("attributes"_cs, new Util::JsonArray());

    tablesJson->append(tableJson);
    LOG2("Added Debug Counter " << tblName);
}


}  // namespace BFRT

}  // namespace BFN
