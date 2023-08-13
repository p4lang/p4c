/* Copyright 2021 Intel Corporation

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
#include "bfruntime.h"

#include <limits>
#include <ostream>
#include <regex>
#include <set>

#include "lib/exceptions.h"
#include "lib/null.h"

namespace P4 {

namespace BFRT {

using P4::BFRT::P4Id;

TypeSpecParser TypeSpecParser::make(const p4configv1::P4Info &p4info,
                                    const p4configv1::P4DataTypeSpec &typeSpec,
                                    cstring instanceType, cstring instanceName,
                                    const std::vector<cstring> *fieldNames, cstring prefix,
                                    cstring suffix, P4Id idOffset) {
    Fields fields;
    const auto &typeInfo = p4info.type_info();

    auto addField = [&](P4Id id, const std::string &name, const p4configv1::P4DataTypeSpec &fSpec) {
        Util::JsonObject *type = nullptr;
        // Add support for required P4DataTypeSpec types here to generate
        // the correct field width
        if (fSpec.has_serializable_enum()) {
            auto enums = typeInfo.serializable_enums();
            auto e = fSpec.serializable_enum().name();
            auto typeEnum = enums.find(e);
            if (typeEnum == enums.end()) {
                ::error(ErrorType::ERR_NOT_FOUND,
                        "Enum type '%1%' not found in typeInfo for '%2%' '%3%'", e, instanceType,
                        instanceName);
                return;
            }
            type = makeTypeBytes(typeEnum->second.underlying_type().bitwidth());
        } else if (fSpec.has_bitstring()) {
            if (fSpec.bitstring().has_bit())
                type = makeTypeBytes(fSpec.bitstring().bit().bitwidth());
            else if (fSpec.bitstring().has_int_())
                type = makeTypeBytes(fSpec.bitstring().int_().bitwidth());
            else if (fSpec.bitstring().has_varbit())
                type = makeTypeBytes(fSpec.bitstring().varbit().max_bitwidth());
        } else if (fSpec.has_bool_()) {
            type = makeTypeBool(1);
        } else if (fSpec.has_new_type()) {
            auto newtypes = typeInfo.new_types();
            auto typeName = fSpec.new_type().name();
            auto newType = newtypes.find(typeName);
            if (newType == newtypes.end()) {
                ::error(ErrorType::ERR_NOT_FOUND,
                        "New type '%1%' not found in typeInfo for '%2%' '%3%'", typeName,
                        instanceType, instanceName);
                return;
            }
            type = makeTypeBytes(newType->second.translated_type().sdn_bitwidth());
        }

        if (!type) {
            ::error(ErrorType::ERR_UNSUPPORTED,
                    "Error when generating BF-RT info for '%1%' '%2%': "
                    "packed type is too complex",
                    instanceType, instanceName);
            return;
        }

        fields.push_back({prefix + name + suffix, id, type});
    };

    if (typeSpec.has_struct_()) {
        auto structName = typeSpec.struct_().name();
        auto p_it = typeInfo.structs().find(structName);
        BUG_CHECK(p_it != typeInfo.structs().end(), "Struct name '%1%' not found in P4Info map",
                  structName);
        P4Id id = idOffset;
        for (const auto &member : p_it->second.members())
            addField(id++, member.name(), member.type_spec());
    } else if (typeSpec.has_tuple()) {
        P4Id id = idOffset;
        int fNameIdx = 0;
        for (const auto &member : typeSpec.tuple().members()) {
            std::string fName;
            if (fieldNames && int(fieldNames->size()) > fNameIdx) {
                fName = (*fieldNames)[fNameIdx++];
            } else {
                // TODO(antonin): we do not really have better names for now, do
                // we need to add support for annotations of tuple members in
                // P4Info?
                fName = "f" + std::to_string(id);
            }
            addField(id++, fName, member);
        }
    } else if (typeSpec.has_bitstring()) {
        // TODO(antonin): same as above, we need a way to pass name
        // annotations
        std::string fName;
        if (fieldNames && fieldNames->size() > 0) {
            fName = (*fieldNames)[0];
        } else {
            // TODO(antonin): we do not really have better names for now, do
            // we need to add support for annotations of tuple members in
            // P4Info?
            fName = "f1";
        }
        addField(idOffset, fName, typeSpec);
    } else if (typeSpec.has_header()) {
        auto headerName = typeSpec.header().name();
        auto p_it = typeInfo.headers().find(headerName);
        BUG_CHECK(p_it != typeInfo.headers().end(), "Header name '%1%' not found in P4Info map",
                  headerName);
        P4Id id = idOffset;
        for (const auto &member : p_it->second.members()) {
            auto *type = makeTypeBytes(member.type_spec().bit().bitwidth());
            fields.push_back({prefix + member.name() + suffix, id++, type});
        }
    } else if (typeSpec.has_serializable_enum()) {
        auto enumName = typeSpec.serializable_enum().name();
        auto p_it = typeInfo.serializable_enums().find(enumName);
        BUG_CHECK(p_it != typeInfo.serializable_enums().end(),
                  "Serializable name '%1%' not found in P4Info map", enumName);
        P4Id id = idOffset;
        for (const auto &member : p_it->second.members()) {
            auto *type = makeTypeBytes(p_it->second.underlying_type().bitwidth());
            fields.push_back({prefix + member.name() + suffix, id++, type});
        }
    } else {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "Error when generating BF-RT info for '%1%' '%2%': "
                "only structs, headers, tuples, bitstrings and "
                "serializable enums are currently supported types",
                instanceType, instanceName);
    }

    return TypeSpecParser(std::move(fields));
}

// Counter
std::optional<BFRuntimeGenerator::Counter> BFRuntimeGenerator::Counter::from(
    const p4configv1::Counter &counterInstance) {
    const auto &pre = counterInstance.preamble();
    auto id = makeBFRuntimeId(pre.id(), p4configv1::P4Ids::COUNTER);
    // TODO(antonin): this works because the enum values are the same for
    // Counter::Unit and for CounterSpec::Unit, but this may not be very
    // future-proof.
    auto unit = static_cast<Unit>(counterInstance.spec().unit());
    return Counter{pre.name(), id, counterInstance.size(), unit, transformAnnotations(pre)};
}

std::optional<BFRuntimeGenerator::Counter> BFRuntimeGenerator::Counter::fromDirect(
    const p4configv1::DirectCounter &counterInstance) {
    const auto &pre = counterInstance.preamble();
    auto id = makeBFRuntimeId(pre.id(), p4configv1::P4Ids::DIRECT_COUNTER);
    auto unit = static_cast<Unit>(counterInstance.spec().unit());
    return Counter{pre.name(), id, 0, unit, transformAnnotations(pre)};
}

// Meter
std::optional<BFRuntimeGenerator::Meter> BFRuntimeGenerator::Meter::from(
    const p4configv1::Meter &meterInstance) {
    const auto &pre = meterInstance.preamble();
    auto id = makeBFRuntimeId(pre.id(), p4configv1::P4Ids::METER);
    auto unit = static_cast<Unit>(meterInstance.spec().unit());
    return Meter{pre.name(), id, meterInstance.size(), unit, transformAnnotations(pre)};
}

std::optional<BFRuntimeGenerator::Meter> BFRuntimeGenerator::Meter::fromDirect(
    const p4configv1::DirectMeter &meterInstance) {
    const auto &pre = meterInstance.preamble();
    auto id = makeBFRuntimeId(pre.id(), p4configv1::P4Ids::DIRECT_METER);
    auto unit = static_cast<Unit>(meterInstance.spec().unit());
    return Meter{pre.name(), id, 0, unit, transformAnnotations(pre)};
}

// Register
std::optional<BFRuntimeGenerator::Register> BFRuntimeGenerator::Register::from(
    const p4configv1::Register &regInstance) {
    const auto &pre = regInstance.preamble();
    auto id = makeBFRuntimeId(pre.id(), p4configv1::P4Ids::REGISTER);
    return Register{pre.name(),         "$REGISTER_INDEX",       id,
                    regInstance.size(), regInstance.type_spec(), transformAnnotations(pre)};
}

// ActionProfile
P4Id BFRuntimeGenerator::ActionProf::makeActProfId(P4Id implementationId) {
    return makeBFRuntimeId(implementationId, p4configv1::P4Ids::ACTION_PROFILE);
}

std::optional<BFRuntimeGenerator::ActionProf> BFRuntimeGenerator::ActionProf::from(
    const p4configv1::P4Info &p4info, const p4configv1::ActionProfile &actionProfile) {
    const auto &pre = actionProfile.preamble();
    auto profileId = makeBFRuntimeId(pre.id(), p4configv1::P4Ids::ACTION_PROFILE);
    auto tableIds =
        collectTableIds(p4info, actionProfile.table_ids().begin(), actionProfile.table_ids().end());
    return ActionProf{pre.name(), profileId, actionProfile.size(), tableIds,
                      transformAnnotations(pre)};
}

// Digest
std::optional<BFRuntimeGenerator::Digest> BFRuntimeGenerator::Digest::from(
    const p4configv1::Digest &digest) {
    const auto &pre = digest.preamble();
    auto id = makeBFRuntimeId(pre.id(), p4configv1::P4Ids::DIGEST);
    return Digest{pre.name(), id, digest.type_spec(), transformAnnotations(pre)};
}

std::optional<BFRuntimeGenerator::Counter> BFRuntimeGenerator::getDirectCounter(
    P4Id counterId) const {
    if (isOfType(counterId, p4configv1::P4Ids::DIRECT_COUNTER)) {
        auto *counter = Standard::findDirectCounter(p4info, counterId);
        if (counter == nullptr) return std::nullopt;
        return Counter::fromDirect(*counter);
    }
    return std::nullopt;
}

std::optional<BFRuntimeGenerator::Meter> BFRuntimeGenerator::getDirectMeter(P4Id meterId) const {
    if (isOfType(meterId, p4configv1::P4Ids::DIRECT_METER)) {
        auto *meter = Standard::findDirectMeter(p4info, meterId);
        if (meter == nullptr) return std::nullopt;
        return Meter::fromDirect(*meter);
    }
    return std::nullopt;
}

// TBD
// std::optional<BFRuntimeGenerator::Register>
// BFRuntimeGenerator::getRegister(P4Id registerId) const {
//     if (!isOfType(registerId, p4configv1::P4Ids::DIRECT_REGISTER)) return std::nullopt;
//     auto* externInstance = findExternInstance(p4info, registerId);
//     if (externInstance == nullptr) return std::nullopt;
//     return Register::from(*externInstance);
// }

Util::JsonObject *BFRuntimeGenerator::makeCommonDataField(P4Id id, cstring name,
                                                          Util::JsonObject *type, bool repeated,
                                                          Util::JsonArray *annotations) {
    auto *dataField = new Util::JsonObject();
    dataField->emplace("id", id);
    dataField->emplace("name", name);
    dataField->emplace("repeated", repeated);
    if (annotations != nullptr)
        dataField->emplace("annotations", annotations);
    else
        dataField->emplace("annotations", new Util::JsonArray());
    dataField->emplace("type", type);
    return dataField;
}

Util::JsonObject *BFRuntimeGenerator::makeContainerDataField(P4Id id, cstring name,
                                                             Util::JsonArray *items, bool repeated,
                                                             Util::JsonArray *annotations) {
    auto *dataField = new Util::JsonObject();
    dataField->emplace("id", id);
    dataField->emplace("name", name);
    dataField->emplace("repeated", repeated);
    if (annotations != nullptr)
        dataField->emplace("annotations", annotations);
    else
        dataField->emplace("annotations", new Util::JsonArray());
    dataField->emplace("container", items);
    return dataField;
}

void BFRuntimeGenerator::addActionDataField(Util::JsonArray *dataJson, P4Id id,
                                            const std::string &name, bool mandatory, bool read_only,
                                            Util::JsonObject *type, Util::JsonArray *annotations) {
    auto *dataField = new Util::JsonObject();
    dataField->emplace("id", id);
    dataField->emplace("name", name);
    dataField->emplace("repeated", false);
    dataField->emplace("mandatory", mandatory);
    dataField->emplace("read_only", read_only);
    if (annotations != nullptr)
        dataField->emplace("annotations", annotations);
    else
        dataField->emplace("annotations", new Util::JsonArray());
    dataField->emplace("type", type);
    dataJson->append(dataField);
}

void BFRuntimeGenerator::addKeyField(Util::JsonArray *dataJson, P4Id id, cstring name,
                                     bool mandatory, cstring matchType, Util::JsonObject *type,
                                     Util::JsonArray *annotations) {
    auto *dataField = new Util::JsonObject();
    dataField->emplace("id", id);
    dataField->emplace("name", name);
    dataField->emplace("repeated", false);
    if (annotations != nullptr)
        dataField->emplace("annotations", annotations);
    else
        dataField->emplace("annotations", new Util::JsonArray());
    dataField->emplace("mandatory", mandatory);
    dataField->emplace("match_type", matchType);
    dataField->emplace("type", type);
    dataJson->append(dataField);
}

/* static */ Util::JsonObject *BFRuntimeGenerator::initTableJson(const std::string &name, P4Id id,
                                                                 cstring tableType, int64_t size,
                                                                 Util::JsonArray *annotations) {
    auto *tableJson = new Util::JsonObject();
    tableJson->emplace("name", name);
    tableJson->emplace("id", id);
    tableJson->emplace("table_type", tableType);
    tableJson->emplace("size", size);
    if (annotations != nullptr) tableJson->emplace("annotations", annotations);
    tableJson->emplace("depends_on", new Util::JsonArray());
    return tableJson;
}

/* static */ void BFRuntimeGenerator::addToDependsOn(Util::JsonObject *tableJson, P4Id id) {
    auto *dependsOnJson = tableJson->get("depends_on")->to<Util::JsonArray>();
    CHECK_NULL(dependsOnJson);
    // Skip duplicates
    for (auto *d : *dependsOnJson) {
        if (*d->to<Util::JsonValue>() == id) return;
    }
    dependsOnJson->append(id);
}

void BFRuntimeGenerator::addCounterCommon(Util::JsonArray *tablesJson,
                                          const Counter &counter) const {
    auto *tableJson =
        initTableJson(counter.name, counter.id, "Counter", counter.size, counter.annotations);

    auto *keyJson = new Util::JsonArray();
    addKeyField(keyJson, TD_DATA_COUNTER_INDEX, "$COUNTER_INDEX", true /* mandatory */, "Exact",
                makeType("uint32"));
    tableJson->emplace("key", keyJson);

    auto *dataJson = new Util::JsonArray();
    addCounterDataFields(dataJson, counter);
    tableJson->emplace("data", dataJson);

    auto *operationsJson = new Util::JsonArray();
    operationsJson->append("Sync");
    tableJson->emplace("supported_operations", operationsJson);

    tableJson->emplace("attributes", new Util::JsonArray());

    tablesJson->append(tableJson);
}

void BFRuntimeGenerator::addCounterDataFields(Util::JsonArray *dataJson,
                                              const BFRuntimeGenerator::Counter &counter) {
    static const uint64_t defaultCounterValue = 0u;
    if (counter.unit == Counter::Unit::BYTES || counter.unit == Counter::Unit::BOTH) {
        auto *f =
            makeCommonDataField(TD_DATA_COUNTER_SPEC_BYTES, "$COUNTER_SPEC_BYTES",
                                makeType("uint64", defaultCounterValue), false /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
    if (counter.unit == Counter::Unit::PACKETS || counter.unit == Counter::Unit::BOTH) {
        auto *f =
            makeCommonDataField(TD_DATA_COUNTER_SPEC_PKTS, "$COUNTER_SPEC_PKTS",
                                makeType("uint64", defaultCounterValue), false /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
}

void BFRuntimeGenerator::addMeterCommon(Util::JsonArray *tablesJson,
                                        const BFRuntimeGenerator::Meter &meter) const {
    auto *tableJson = initTableJson(meter.name, meter.id, "Meter", meter.size);

    auto *keyJson = new Util::JsonArray();
    addKeyField(keyJson, TD_DATA_METER_INDEX, "$METER_INDEX", true /* mandatory */, "Exact",
                makeType("uint32"));
    tableJson->emplace("key", keyJson);

    auto *dataJson = new Util::JsonArray();
    addMeterDataFields(dataJson, meter);
    tableJson->emplace("data", dataJson);

    tableJson->emplace("supported_operations", new Util::JsonArray());

    auto *attributesJson = new Util::JsonArray();
    attributesJson->append("MeterByteCountAdjust");
    tableJson->emplace("attributes", attributesJson);

    tablesJson->append(tableJson);
}

void BFRuntimeGenerator::transformTypeSpecToDataFields(Util::JsonArray *fieldsJson,
                                                       const p4configv1::P4DataTypeSpec &typeSpec,
                                                       cstring instanceType, cstring instanceName,
                                                       const std::vector<cstring> *fieldNames,
                                                       cstring prefix, cstring suffix,
                                                       P4Id idOffset) const {
    auto parser = TypeSpecParser::make(p4info, typeSpec, instanceType, instanceName, fieldNames,
                                       prefix, suffix, idOffset);
    for (const auto &f : parser) {
        auto *fJson = makeCommonDataField(f.id, f.name, f.type, false /* repeated */);
        fieldsJson->append(fJson);
    }
}

void BFRuntimeGenerator::addRegisterDataFields(Util::JsonArray *dataJson,
                                               const BFRuntimeGenerator::Register &register_,
                                               P4Id idOffset) const {
    auto parser = TypeSpecParser::make(p4info, register_.typeSpec, "Register", register_.name,
                                       nullptr, "", "", idOffset);

    BUG_CHECK(parser.size() == 1, "Expected only one data field for Register extern %1%",
              register_.name);
    for (const auto &f : parser) {
        auto *fJson =
            makeCommonDataField(idOffset, "$REGISTER_INDEX", f.type, false /* repeated */);
        addSingleton(dataJson, fJson, false /* mandatory */, false /* read-only */);
    }
}

void BFRuntimeGenerator::addRegisterCommon(Util::JsonArray *tablesJson,
                                           const BFRuntimeGenerator::Register &register_) const {
    auto *tableJson = initTableJson(register_.name, register_.id, "Register", register_.size,
                                    register_.annotations);
    auto *keyJson = new Util::JsonArray();
    addKeyField(keyJson, TD_DATA_REGISTER_INDEX, "$REGISTER_INDEX", true /* mandatory */, "Exact",
                makeType("uint32"));
    tableJson->emplace("key", keyJson);

    auto *dataJson = new Util::JsonArray();
    addRegisterDataFields(dataJson, register_, TD_DATA_REGISTER_INDEX);
    tableJson->emplace("data", dataJson);

    auto *operationsJson = new Util::JsonArray();
    operationsJson->append("Sync");
    tableJson->emplace("supported_operations", operationsJson);

    tableJson->emplace("attributes", new Util::JsonArray());

    tablesJson->append(tableJson);
}

void BFRuntimeGenerator::addMeterDataFields(Util::JsonArray *dataJson,
                                            const BFRuntimeGenerator::Meter &meter) {
    // default value for rates and bursts (all GREEN)
    static const uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    if (meter.unit == Meter::Unit::BYTES) {
        {
            auto *f = makeCommonDataField(TD_DATA_METER_SPEC_CIR_KBPS, "$METER_SPEC_CIR_KBPS",
                                          makeType("uint64", maxUint64), false /* repeated */);
            addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
        }
        {
            auto *f = makeCommonDataField(TD_DATA_METER_SPEC_PIR_KBPS, "$METER_SPEC_PIR_KBPS",
                                          makeType("uint64", maxUint64), false /* repeated */);
            addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
        }
        {
            auto *f = makeCommonDataField(TD_DATA_METER_SPEC_CBS_KBITS, "$METER_SPEC_CBS_KBITS",
                                          makeType("uint64", maxUint64), false /* repeated */);
            addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
        }
        {
            auto *f = makeCommonDataField(TD_DATA_METER_SPEC_PBS_KBITS, "$METER_SPEC_PBS_KBITS",
                                          makeType("uint64", maxUint64), false /* repeated */);
            addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
        }
    } else if (meter.unit == Meter::Unit::PACKETS) {
        {
            auto *f = makeCommonDataField(TD_DATA_METER_SPEC_CIR_PPS, "$METER_SPEC_CIR_PPS",
                                          makeType("uint64", maxUint64), false /* repeated */);
            addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
        }
        {
            auto *f = makeCommonDataField(TD_DATA_METER_SPEC_PIR_PPS, "$METER_SPEC_PIR_PPS",
                                          makeType("uint64", maxUint64), false /* repeated */);
            addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
        }
        {
            auto *f = makeCommonDataField(TD_DATA_METER_SPEC_CBS_PKTS, "$METER_SPEC_CBS_PKTS",
                                          makeType("uint64", maxUint64), false /* repeated */);
            addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
        }
        {
            auto *f = makeCommonDataField(TD_DATA_METER_SPEC_PBS_PKTS, "$METER_SPEC_PBS_PKTS",
                                          makeType("uint64", maxUint64), false /* repeated */);
            addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
        }
    } else {
        BUG("Unknown meter unit");
    }
}

void BFRuntimeGenerator::addActionProfCommon(
    Util::JsonArray *tablesJson, const BFRuntimeGenerator::ActionProf &actionProf) const {
    auto *tableJson = initTableJson(actionProf.name, actionProf.id, "Action", actionProf.size,
                                    actionProf.annotations);

    auto *keyJson = new Util::JsonArray();
    addKeyField(keyJson, TD_DATA_ACTION_MEMBER_ID, "$ACTION_MEMBER_ID", true /* mandatory */,
                "Exact", makeType("uint32"));
    tableJson->emplace("key", keyJson);

    if (actionProf.tableIds.empty()) {
        ::warning("Action profile '%1%' is not used by any table, skipping it", actionProf.name);
        return;
    }
    auto oneTableId = actionProf.tableIds.at(0);
    auto *oneTable = Standard::findTable(p4info, oneTableId);
    CHECK_NULL(oneTable);

    // Add action profile to match table depends on
    auto oneTableJson = findJsonTable(tablesJson, oneTable->preamble().name());
    addToDependsOn(oneTableJson, actionProf.id);

    tableJson->emplace("action_specs", makeActionSpecs(*oneTable));

    tableJson->emplace("data", new Util::JsonArray());

    tableJson->emplace("supported_operations", new Util::JsonArray());
    tableJson->emplace("attributes", new Util::JsonArray());

    tablesJson->append(tableJson);
}

std::optional<bool> BFRuntimeGenerator::actProfHasSelector(P4Id actProfId) const {
    if (isOfType(actProfId, p4configv1::P4Ids::ACTION_PROFILE)) {
        auto *actionProf = Standard::findActionProf(p4info, actProfId);
        if (actionProf == nullptr) return std::nullopt;
        return actionProf->with_selector();
    }
    return std::nullopt;
}

Util::JsonArray *BFRuntimeGenerator::makeActionSpecs(const p4configv1::Table &table,
                                                     P4Id *maxActionParamId) const {
    auto *specs = new Util::JsonArray();
    P4Id maxId = 0;
    for (const auto &action_ref : table.action_refs()) {
        auto *action = Standard::findAction(p4info, action_ref.id());
        if (action == nullptr) {
            ::error(ErrorType::ERR_INVALID, "Invalid action id '%1%'", action_ref.id());
            continue;
        }
        auto *spec = new Util::JsonObject();
        const auto &pre = action->preamble();
        spec->emplace("id", pre.id());
        spec->emplace("name", pre.name());
        switch (action_ref.scope()) {
            case p4configv1::ActionRef::TABLE_AND_DEFAULT:
                spec->emplace("action_scope", "TableAndDefault");
                break;
            case p4configv1::ActionRef::TABLE_ONLY:
                spec->emplace("action_scope", "TableOnly");
                break;
            case p4configv1::ActionRef::DEFAULT_ONLY:
                spec->emplace("action_scope", "DefaultOnly");
                break;
            default:
                ::error(ErrorType::ERR_INVALID, "Invalid action ref scope '%1%' in P4Info",
                        int(action_ref.scope()));
                break;
        }
        auto *annotations =
            transformAnnotations(action_ref.annotations().begin(), action_ref.annotations().end());
        spec->emplace("annotations", annotations);

        auto *dataJson = new Util::JsonArray();
        for (const auto &param : action->params()) {
            auto *annotations =
                transformAnnotations(param.annotations().begin(), param.annotations().end());
            if (param.type_name().name() == "PSA_MeterColor_t") {
                addActionDataField(dataJson, param.id(), param.name(), true /* mandatory */,
                                   false /* read_only */, makeTypeEnum({"RED", "GREEN", "YELLOW"}),
                                   annotations);
            } else {
                addActionDataField(dataJson, param.id(), param.name(), true /* mandatory */,
                                   false /* read_only */, makeTypeBytes(param.bitwidth()),
                                   annotations);
            }
            if (param.id() > maxId) maxId = param.id();
        }
        spec->emplace("data", dataJson);
        specs->append(spec);
    }
    if (maxActionParamId != nullptr) *maxActionParamId = maxId;
    return specs;
}

void BFRuntimeGenerator::addLearnFilterCommon(Util::JsonArray *learnFiltersJson,
                                              const BFRuntimeGenerator::Digest &digest) const {
    auto *learnFilterJson = new Util::JsonObject();
    learnFilterJson->emplace("name", digest.name);
    learnFilterJson->emplace("id", digest.id);
    learnFilterJson->emplace("annotations", digest.annotations);

    auto *fieldsJson = new Util::JsonArray();
    transformTypeSpecToDataFields(fieldsJson, digest.typeSpec, "Digest", digest.name);
    learnFilterJson->emplace("fields", fieldsJson);

    learnFiltersJson->append(learnFilterJson);
}

void BFRuntimeGenerator::addLearnFilters(Util::JsonArray *learnFiltersJson) const {
    for (const auto &digest : p4info.digests()) {
        auto digestInstance = Digest::from(digest);
        if (digestInstance == std::nullopt) continue;
        addLearnFilterCommon(learnFiltersJson, *digestInstance);
    }
}

void BFRuntimeGenerator::addDirectResources(const p4configv1::Table &table,
                                            Util::JsonArray *dataJson,
                                            Util::JsonArray *operationsJson,
                                            Util::JsonArray *attributesJson, P4Id) const {
    // direct resources
    for (auto directResId : table.direct_resource_ids()) {
        if (auto counter = getDirectCounter(directResId)) {
            addCounterDataFields(dataJson, *counter);
            operationsJson->append("SyncCounters");
        } else if (auto meter = getDirectMeter(directResId)) {
            addMeterDataFields(dataJson, *meter);
            attributesJson->append("MeterByteCountAdjust");
        } else {
            ::error(ErrorType::ERR_UNKNOWN, "Unknown direct resource id '%1%'", directResId);
            continue;
        }
    }
}

bool BFRuntimeGenerator::addActionProfIds(const p4configv1::Table &table,
                                          Util::JsonObject *tableJson) const {
    auto implementationId = table.implementation_id();
    if (implementationId > 0) {
        P4Id actProfId = ActionProf::makeActProfId(implementationId);
        addToDependsOn(tableJson, actProfId);
    }
    return true;
}

void BFRuntimeGenerator::addConstTableAttr(Util::JsonArray *attrJson) const {
    attrJson->append("ConstTable");
}

bool BFRuntimeGenerator::addMatchTypePriority(std::optional<cstring> &matchType) const {
    if (*matchType == "Ternary" || *matchType == "Range" || *matchType == "Optional") {
        return true;
    }
    return false;
}

void BFRuntimeGenerator::addMatchTables(Util::JsonArray *tablesJson) const {
    for (const auto &table : p4info.tables()) {
        const auto &pre = table.preamble();
        std::set<std::string> dupKey;

        auto *annotations =
            transformAnnotations(pre.annotations().begin(), pre.annotations().end());

        auto *tableJson =
            initTableJson(pre.name(), pre.id(), "MatchAction_Direct", table.size(), annotations);

        if (!addActionProfIds(table, tableJson)) continue;

        tableJson->emplace("has_const_default_action", table.const_default_action_id() != 0);

        // will be set to true by the for loop if the match key includes a
        // ternary, range or optional match
        bool needsPriority = false;
        auto *keyJson = new Util::JsonArray();
        for (const auto &mf : table.match_fields()) {
            std::optional<cstring> matchType = std::nullopt;
            switch (mf.match_case()) {
                case p4configv1::MatchField::kMatchType:
                    matchType = transformMatchType(mf.match_type());
                    break;
                case p4configv1::MatchField::kOtherMatchType:
                    matchType = transformOtherMatchType(mf.other_match_type());
                    break;
                default:
                    BUG("Invalid oneof case for the match type of table '%1%'", pre.name());
                    break;
            }
            if (matchType == std::nullopt) {
                ::error(ErrorType::ERR_UNSUPPORTED, "Unsupported match type for BF-RT: %1%",
                        int(mf.match_type()));
                continue;
            }
            needsPriority |= addMatchTypePriority(matchType);
            auto *annotations =
                transformAnnotations(mf.annotations().begin(), mf.annotations().end());

            CHECK_NULL(annotations);
            // Add isFieldSlice annotation if the match key is a field slice
            // e.g. hdr[23:16] where hdr is a 32 bit field
            std::string s(mf.name());
            std::smatch sm;
            std::regex sliceRegex(R"(\[([0-9]+):([0-9]+)\])");
            std::regex_search(s, sm, sliceRegex);
            if (sm.size() == 3) {
                auto *isFieldSliceAnnot = new Util::JsonObject();
                isFieldSliceAnnot->emplace("name", "isFieldSlice");
                isFieldSliceAnnot->emplace("value", "true");
                annotations->append(isFieldSliceAnnot);
            }

            // P4_14-->P4_16 translation names valid matches with a
            // "$valid$" suffix (note the trailing "$").  However, Brig
            // and pdgen use "$valid".
            auto keyName = mf.name();
            auto found = keyName.find(".$valid");
            if (found != std::string::npos) keyName.pop_back();

            // Replace header stack indices hdr[<index>] with hdr$<index>. This
            // is output in the context.json
            std::regex hdrStackRegex(R"(\[([0-9]+)\])");
            keyName = std::regex_replace(keyName, hdrStackRegex, "$$$1");

            // Control plane requires there's no duplicate key in one table.
            if (dupKey.count(keyName) != 0) {
                ::error(ErrorType::ERR_DUPLICATE,
                        "Key \"%s\" is duplicate in Table \"%s\". It doesn't meet BFRT's "
                        "requirements.\n"
                        "Please using @name annotation for the duplicate key names",
                        keyName, pre.name());
                return;
            } else {
                dupKey.insert(keyName);
            }

            // DRV-3112 - Make key fields not mandatory, this allows user to use a
            // driver initialized default value (0).
            addKeyField(keyJson, mf.id(), keyName, false /* mandatory */, *matchType,
                        makeTypeBytes(mf.bitwidth(), std::nullopt), annotations);
        }
        if (needsPriority) {
            // DRV-3112 - Make key fields not mandatory, this allows user to use a
            // driver initialized default value (0).
            addKeyField(keyJson, TD_DATA_MATCH_PRIORITY, "$MATCH_PRIORITY", false /* mandatory */,
                        "Exact", makeType("uint32"));
        }
        tableJson->emplace("key", keyJson);

        auto *dataJson = new Util::JsonArray();

        // will be used as an offset for other P4-dependent fields (e.g. direct
        // register fields).
        P4Id maxActionParamId = 0;
        cstring tableType = tableJson->get("table_type")->to<Util::JsonValue>()->getString();
        if (tableType == "MatchAction_Direct") {
            tableJson->emplace("action_specs", makeActionSpecs(table, &maxActionParamId));
        } else if (tableType == "MatchAction_Indirect") {
            auto *f = makeCommonDataField(TD_DATA_ACTION_MEMBER_ID, "$ACTION_MEMBER_ID",
                                          makeType("uint32"), false /* repeated */);
            addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
        } else if (tableType == "MatchAction_Indirect_Selector") {
            // action member id and selector group id are mutually-exclusive, so
            // we use a "oneof" here.
            auto *choicesDataJson = new Util::JsonArray();
            choicesDataJson->append(makeCommonDataField(TD_DATA_ACTION_MEMBER_ID,
                                                        "$ACTION_MEMBER_ID", makeType("uint32"),
                                                        false /* repeated */));
            choicesDataJson->append(makeCommonDataField(TD_DATA_SELECTOR_GROUP_ID,
                                                        "$SELECTOR_GROUP_ID", makeType("uint32"),
                                                        false /* repeated */));
            addOneOf(dataJson, choicesDataJson, true /* mandatory */, false /* read-only */);
        } else {
            BUG("Invalid table type '%1%'", tableType);
        }
        maxActionParamId++;

        auto *operationsJson = new Util::JsonArray();
        auto *attributesJson = new Util::JsonArray();

        addDirectResources(table, dataJson, operationsJson, attributesJson, maxActionParamId);

        attributesJson->append("EntryScope");

        // The compiler backend is in charge of rejecting the program if
        // @dynamic_table_key_masks is used incorrectly (e.g. if the table is
        // ternary).
        auto supportsDynMask = std::count(pre.annotations().begin(), pre.annotations().end(),
                                          "@dynamic_table_key_masks(1)");
        if (supportsDynMask) attributesJson->append("DynamicKeyMask");

        if (table.is_const_table()) addConstTableAttr(attributesJson);
        if (table.idle_timeout_behavior() == p4configv1::Table::NOTIFY_CONTROL) {
            auto pollModeOnly = std::count(pre.annotations().begin(), pre.annotations().end(),
                                           "@idletime_precision(1)");

            operationsJson->append("UpdateHitState");
            attributesJson->append("IdleTimeout");

            auto *fEntryTTL = makeCommonDataField(
                TD_DATA_ENTRY_TTL, "$ENTRY_TTL",
                makeType("uint32", 0 /* default TTL -> ageing disabled */), false /* repeated */);
            auto *fEntryHitState = makeCommonDataField(TD_DATA_ENTRY_HIT_STATE, "$ENTRY_HIT_STATE",
                                                       makeTypeEnum({"ENTRY_IDLE", "ENTRY_ACTIVE"}),
                                                       false /* repeated */);
            addSingleton(dataJson, fEntryHitState, false /* mandatory */, false /* read-only */);
            if (!pollModeOnly)
                addSingleton(dataJson, fEntryTTL, false /* mandatory */, false /* read-only */);
        }

        tableJson->emplace("data", dataJson);
        tableJson->emplace("supported_operations", operationsJson);
        tableJson->emplace("attributes", attributesJson);

        tablesJson->append(tableJson);
    }
}

void BFRuntimeGenerator::addActionProfs(Util::JsonArray *tablesJson) const {
    for (const auto &actionProf : p4info.action_profiles()) {
        auto actionProfInstance = ActionProf::from(p4info, actionProf);
        if (actionProfInstance == std::nullopt) continue;
        addActionProfCommon(tablesJson, *actionProfInstance);
    }
}

void BFRuntimeGenerator::addCounters(Util::JsonArray *tablesJson) const {
    for (const auto &counter : p4info.counters()) {
        auto counterInstance = Counter::from(counter);
        if (counterInstance == std::nullopt) continue;
        addCounterCommon(tablesJson, *counterInstance);
    }
}

void BFRuntimeGenerator::addMeters(Util::JsonArray *tablesJson) const {
    for (const auto &meter : p4info.meters()) {
        auto meterInstance = Meter::from(meter);
        if (meterInstance == std::nullopt) continue;
        addMeterCommon(tablesJson, *meterInstance);
    }
}

void BFRuntimeGenerator::addRegisters(Util::JsonArray *tablesJson) const {
    for (const auto &reg : p4info.registers()) {
        auto regInstance = Register::from(reg);
        if (regInstance == std::nullopt) continue;
        addRegisterCommon(tablesJson, *regInstance);
    }
}

const Util::JsonObject *BFRuntimeGenerator::genSchema() const {
    auto *json = new Util::JsonObject();

    json->emplace("schema_version", cstring("1.0.0"));

    auto *tablesJson = new Util::JsonArray();
    json->emplace("tables", tablesJson);

    addMatchTables(tablesJson);
    addActionProfs(tablesJson);
    addCounters(tablesJson);
    addMeters(tablesJson);
    addRegisters(tablesJson);

    auto *learnFiltersJson = new Util::JsonArray();
    json->emplace("learn_filters", learnFiltersJson);
    addLearnFilters(learnFiltersJson);

    return json;
}

void BFRuntimeGenerator::serializeBFRuntimeSchema(std::ostream *destination) {
    auto *json = genSchema();
    json->serialize(*destination);
    destination->flush();
}

}  // namespace BFRT

}  // namespace P4
