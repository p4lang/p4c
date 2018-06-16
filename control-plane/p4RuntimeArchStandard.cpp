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

#include <set>
#include <unordered_map>

#include <boost/optional.hpp>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "typeSpecConverter.h"

#include "p4RuntimeArchHandler.h"

using ::P4::ControlPlaneAPI::Helpers::getExternInstanceFromProperty;
using ::P4::ControlPlaneAPI::Helpers::addAnnotations;

namespace p4configv1 = ::p4::config::v1;

namespace P4 {

/** \addtogroup control_plane
 *  @{
 */
namespace ControlPlaneAPI {

namespace Standard {

/// v1model counter extern type
struct CounterExtern { };
/// v1model meter extern type
struct MeterExtern { };

}  // namespace Standard

namespace Helpers {

// According to the C++11 standard: An explicit specialization shall be declared
// in a namespace enclosing the specialized template. An explicit specialization
// whose declarator-id is not qualified shall be declared in the nearest
// enclosing namespace of the template, or, if the namespace is inline (7.3.1),
// any namespace from its enclosing namespace set. Such a declaration may also
// be a definition. If the declaration is not a definition, the specialization
// may be defined later (7.3.1.2).
//
// gcc reports an error when trying so specialize CounterlikeTraits<> for
// Standard::CounterExtern & Standard::MeterExtern outside of the Helpers
// namespace, even when qualifying CounterlikeTraits<> with Helpers::. It seems
// to be related to thsi bug:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56480.

/// @ref CounterlikeTraits<> specialization for @ref CounterExtern
template<> struct CounterlikeTraits<Standard::CounterExtern> {
    static const cstring name() { return "counter"; }
    static const cstring directPropertyName() {
        return P4V1::V1Model::instance.tableAttributes.counters.name;
    }
    static const cstring typeName() {
        return P4V1::V1Model::instance.counter.name;
    }
    static const cstring directTypeName() {
        return P4V1::V1Model::instance.directCounter.name;
    }
};

/// @ref CounterlikeTraits<> specialization for @ref MeterExtern
template<> struct CounterlikeTraits<Standard::MeterExtern> {
    static const cstring name() { return "meter"; }
    static const cstring directPropertyName() {
        return P4V1::V1Model::instance.tableAttributes.meters.name;
    }
    static const cstring typeName() {
        return P4V1::V1Model::instance.meter.name;
    }
    static const cstring directTypeName() {
        return P4V1::V1Model::instance.directMeter.name;
    }
};

}  // namespace Helpers

namespace Standard {

/// Extends @ref P4RuntimeSymbolType for the standard (v1model & PSA) extern
/// types.
class SymbolType : public P4RuntimeSymbolType {
 public:
    SymbolType() = delete;

    static P4RuntimeSymbolType ACTION_PROFILE() {
        return P4RuntimeSymbolType::make(p4configv1::P4Ids::ACTION_PROFILE);
    }
    static P4RuntimeSymbolType COUNTER() {
        return P4RuntimeSymbolType::make(p4configv1::P4Ids::COUNTER);
    }
    static P4RuntimeSymbolType DIGEST() {
        return P4RuntimeSymbolType::make(p4configv1::P4Ids::DIGEST);
    }
    static P4RuntimeSymbolType DIRECT_COUNTER() {
        return P4RuntimeSymbolType::make(p4configv1::P4Ids::DIRECT_COUNTER);
    }
    static P4RuntimeSymbolType DIRECT_METER() {
        return P4RuntimeSymbolType::make(p4configv1::P4Ids::DIRECT_METER);
    }
    static P4RuntimeSymbolType METER() {
        return P4RuntimeSymbolType::make(p4configv1::P4Ids::METER);
    }
    static P4RuntimeSymbolType REGISTER() {
        return P4RuntimeSymbolType::make(p4configv1::P4Ids::REGISTER);
    }
};

/// The information about a digest call which is needed to serialize it.
struct Digest {
    const cstring name;       // The fully qualified external name of the digest
                              // *data* - in P4-14, the field list name, or in
                              // P4-16, the type of the 'data' parameter.
    const p4configv1::P4DataTypeSpec* typeSpec;  // The format of the packed data.
};

struct Register {
    const cstring name;  // The fully qualified external name of this register.
    const IR::IAnnotated* annotations;  // If non-null, any annotations applied
                                        // to this field.
    const int64_t size;
    const p4configv1::P4DataTypeSpec* typeSpec;  // The format of the stored data.

    /// @return the information required to serialize an @instance of register
    /// or boost::none in case of error.
    static boost::optional<Register>
    from(const IR::ExternBlock* instance,
         const ReferenceMap* refMap,
         const TypeMap* typeMap,
         p4configv1::P4TypeInfo* p4RtTypeInfo) {
        CHECK_NULL(instance);
        auto declaration = instance->node->to<IR::Declaration_Instance>();

        auto size = instance->getParameterValue("size")->to<IR::Constant>();
        if (!size->is<IR::Constant>()) {
            ::error("Register '%1%' has a non-constant size: %2%", declaration, size);
            return boost::none;
        }
        if (!size->to<IR::Constant>()->fitsInt()) {
            ::error("Register '%1%' has a size that doesn't fit in an integer: %2%",
                    declaration, size);
            return boost::none;
        }

        // retrieve type parameter for the register instance and convert it to P4DataTypeSpec
        BUG_CHECK(declaration->type->is<IR::Type_Specialized>(),
                  "%1%: expected Type_Specialized", declaration->type);
        auto type = declaration->type->to<IR::Type_Specialized>();
        BUG_CHECK(type->arguments->size() == 1,
                  "%1%: expected one type argument", instance);
        auto typeArg = type->arguments->at(0);
        auto typeSpec = TypeSpecConverter::convert(typeMap, refMap, typeArg, p4RtTypeInfo);
        CHECK_NULL(typeSpec);

        return Register{declaration->controlPlaneName(),
                        declaration->to<IR::IAnnotated>(),
                        size->value.get_si(),
                        typeSpec};
    }
};

/// The types of action profiles available in v1model & PSA.
enum class ActionProfileType {
    INDIRECT,
    INDIRECT_WITH_SELECTOR
};

/// The information about an action profile which is necessary to generate its
/// serialized representation.
struct ActionProfile {
    const cstring name;  // The fully qualified external name of this action profile.
    const ActionProfileType type;
    const int64_t size;
    const IR::IAnnotated* annotations;  // If non-null, any annotations applied to this action
                                        // profile declaration.

    bool operator<(const ActionProfile& other) const {
        if (name != other.name) return name < other.name;
        if (type != other.type) return type < other.type;
        return size < other.size;
    }
};

/// Implements @ref P4RuntimeArchHandlerIface for the v1model architecture. The
/// overridden metods will be called by the @P4RuntimeSerializer to collect and
/// serialize v1model-specific symbols which are exposed to the control-plane.
class P4RuntimeArchHandlerV1Model final : public P4RuntimeArchHandlerIface {
 public:
    using Counter = p4configv1::Counter;
    using Meter = p4configv1::Meter;
    using CounterSpec = p4configv1::CounterSpec;
    using MeterSpec = p4configv1::MeterSpec;

    P4RuntimeArchHandlerV1Model(ReferenceMap* refMap,
                                TypeMap* typeMap,
                                const IR::ToplevelBlock* evaluatedProgram)
        : refMap(refMap), typeMap(typeMap), evaluatedProgram(evaluatedProgram) { }

    void collectTableProperties(P4RuntimeSymbolTableIface* symbols,
                                const IR::TableBlock* tableBlock) override {
        CHECK_NULL(tableBlock);
        auto table = tableBlock->container;
        bool isConstructedInPlace = false;

        {
            auto instance = getExternInstanceFromProperty(
                table,
                P4V1::V1Model::instance.tableAttributes.tableImplementation.name,
                refMap,
                typeMap,
                &isConstructedInPlace);
            if (instance) {
                if (instance->type->name != P4V1::V1Model::instance.action_profile.name &&
                    instance->type->name != P4V1::V1Model::instance.action_selector.name) {
                    ::error("Expected an action profile or action selector: %1%",
                            instance->expression);
                } else if (isConstructedInPlace) {
                    symbols->add(SymbolType::ACTION_PROFILE(), *instance->name);
                }
            }
        }
        {
            auto instance = getExternInstanceFromProperty(
                table,
                P4V1::V1Model::instance.tableAttributes.counters.name,
                refMap,
                typeMap,
                &isConstructedInPlace);
            if (instance) {
                if (instance->type->name != P4V1::V1Model::instance.directCounter.name) {
                    ::error("Expected a direct counter: %1%", instance->expression);
                } else if (isConstructedInPlace) {
                    symbols->add(SymbolType::DIRECT_COUNTER(), *instance->name);
                }
            }
        }
        {
            auto instance = getExternInstanceFromProperty(
                table,
                P4V1::V1Model::instance.tableAttributes.meters.name,
                refMap,
                typeMap,
                &isConstructedInPlace);
            if (instance) {
                if (instance->type->name != P4V1::V1Model::instance.directMeter.name) {
                    ::error("Expected a direct meter: %1%", instance->expression);
                } else if (isConstructedInPlace) {
                    symbols->add(SymbolType::DIRECT_METER(), *instance->name);
                }
            }
        }
    }

    void collectExternInstance(P4RuntimeSymbolTableIface* symbols,
                               const IR::ExternBlock* externBlock) override {
        CHECK_NULL(externBlock);

        auto decl = externBlock->node->to<IR::IDeclaration>();
        if (decl == nullptr) return;

        if (externBlock->type->name == P4V1::V1Model::instance.counter.name) {
            symbols->add(SymbolType::COUNTER(), decl);
        } else if (externBlock->type->name == P4V1::V1Model::instance.directCounter.name) {
            symbols->add(SymbolType::DIRECT_COUNTER(), decl);
        } else if (externBlock->type->name == P4V1::V1Model::instance.meter.name) {
            symbols->add(SymbolType::METER(), decl);
        } else if (externBlock->type->name == P4V1::V1Model::instance.directMeter.name) {
            symbols->add(SymbolType::DIRECT_METER(), decl);
        } else if (externBlock->type->name == P4V1::V1Model::instance.action_profile.name ||
                   externBlock->type->name == P4V1::V1Model::instance.action_selector.name) {
            symbols->add(SymbolType::ACTION_PROFILE(), decl);
        } else if (externBlock->type->name == P4V1::V1Model::instance.registers.name) {
            symbols->add(SymbolType::REGISTER(), decl);
        }
    }

    void collectExternFunction(P4RuntimeSymbolTableIface* symbols,
                               const P4::ExternFunction* externFunction) override {
        auto digest = getDigestCall(externFunction, refMap, typeMap, nullptr);
        if (digest) symbols->add(SymbolType::DIGEST(), digest->name);
    }

    void postCollect(const P4RuntimeSymbolTableIface& symbols) override {
        (void)symbols;
        // analyze action profiles and build a mapping from action profile name
        // to the set of tables referencing them
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (!block->is<IR::TableBlock>()) return;
            auto table = block->to<IR::TableBlock>()->container;
            auto implementation = getTableImplementationName(table, refMap);
            if (implementation)
                actionProfilesRefs[*implementation].insert(table->controlPlaneName());
        });
    }

    void addTableProperties(const P4RuntimeSymbolTableIface& symbols,
                            p4configv1::P4Info* p4info,
                            p4configv1::Table* table,
                            const IR::TableBlock* tableBlock) override {
        CHECK_NULL(tableBlock);
        auto tableDeclaration = tableBlock->container;

        using Helpers::isExternPropertyConstructedInPlace;

        auto implementation = getActionProfile(tableDeclaration, refMap, typeMap);
        auto directCounter = Helpers::getDirectCounterlike<CounterExtern>(
            tableDeclaration, refMap, typeMap);
        auto directMeter = Helpers::getDirectCounterlike<MeterExtern>(
            tableDeclaration, refMap, typeMap);
        bool supportsTimeout = getSupportsTimeout(tableDeclaration);

        if (implementation) {
            auto id = symbols.getId(SymbolType::ACTION_PROFILE(),
                                    implementation->name);
            table->set_implementation_id(id);
            auto propertyName = P4V1::V1Model::instance.tableAttributes.tableImplementation.name;
            if (isExternPropertyConstructedInPlace(tableDeclaration, propertyName))
                addActionProfile(symbols, p4info, *implementation);
        }

        if (directCounter) {
            auto id = symbols.getId(SymbolType::DIRECT_COUNTER(),
                                    directCounter->name);
            table->add_direct_resource_ids(id);
            // no risk to add twice because direct counters cannot be shared
            // auto propertyName = P4V1::V1Model::instance.tableAttributes.counters.name;
            // if (isExternPropertyConstructedInPlace(tableDeclaration, propertyName))
            //     addCounter(symbols, p4info, *directCounter);
            addCounter(symbols, p4info, *directCounter);
        }

        if (directMeter) {
            auto id = symbols.getId(SymbolType::DIRECT_METER(),
                                    directMeter->name);
            table->add_direct_resource_ids(id);
            // no risk to add twice because direct meters cannot be shared
            // auto propertyName = P4V1::V1Model::instance.tableAttributes.meters.name;
            // if (isExternPropertyConstructedInPlace(tableDeclaration, propertyName))
            //     addMeter(symbols, p4info, *directMeter);
            addMeter(symbols, p4info, *directMeter);
        }

        if (supportsTimeout) {
            table->set_idle_timeout_behavior(p4configv1::Table::NOTIFY_CONTROL);
        } else {
            table->set_idle_timeout_behavior(p4configv1::Table::NO_TIMEOUT);
        }
    }

    void addExternInstance(const P4RuntimeSymbolTableIface& symbols,
                           p4configv1::P4Info* p4info,
                           const IR::ExternBlock* externBlock) override {
        auto decl = externBlock->node->to<IR::Declaration_Instance>();
        if (decl == nullptr) return;

        auto p4RtTypeInfo = p4info->mutable_type_info();
        if (externBlock->type->name == Helpers::CounterlikeTraits<CounterExtern>::typeName()) {
            auto counter = Helpers::Counterlike<CounterExtern>::from(externBlock);
            if (counter) addCounter(symbols, p4info, *counter);
        } else if (externBlock->type->name == Helpers::CounterlikeTraits<MeterExtern>::typeName()) {
            auto meter = Helpers::Counterlike<MeterExtern>::from(externBlock);
            if (meter) addMeter(symbols, p4info, *meter);
        } else if (externBlock->type->name == P4V1::V1Model::instance.registers.name) {
            auto register_ = Register::from(externBlock, refMap, typeMap, p4RtTypeInfo);
            if (register_) addRegister(symbols, p4info, *register_);
        } else if (externBlock->type->name == P4V1::V1Model::instance.action_profile.name ||
                   externBlock->type->name == P4V1::V1Model::instance.action_selector.name) {
            auto actionProfile = getActionProfile(decl, externBlock->type);
            if (actionProfile) addActionProfile(symbols, p4info, *actionProfile);
        }
    }

    void addExternFunction(const P4RuntimeSymbolTableIface& symbols,
                           p4configv1::P4Info* p4info,
                           const P4::ExternFunction* externFunction) override {
        auto p4RtTypeInfo = p4info->mutable_type_info();
        auto digest = getDigestCall(externFunction, refMap, typeMap, p4RtTypeInfo);
        if (digest) addDigest(symbols, p4info, *digest);
    }

    /// @return serialization information for the digest() call represented by
    /// @call, or boost::none if @call is not a digest() call or is invalid.
    static boost::optional<Digest>
    getDigestCall(const P4::ExternFunction* function,
                  ReferenceMap* refMap,
                  TypeMap* typeMap,
                  p4configv1::P4TypeInfo* p4RtTypeInfo) {
        if (function->method->name != P4V1::V1Model::instance.digest_receiver.name)
            return boost::none;

        auto call = function->expr;
        BUG_CHECK(call->typeArguments->size() == 1,
                  "%1%: Expected one type argument", call);
        BUG_CHECK(call->arguments->size() == 2, "%1%: Expected 2 arguments", call);

        // An invocation of digest() looks like this:
        //   digest<T>(receiver, { fields });
        // The name that shows up in the control plane API is the type name T. If T
        // doesn't have a name (e.g. tuple), we auto-generate one; ideally we would
        // be able to annotate the digest method call with a @name annotation in the
        // P4 but annotations are not supported on expressions.
        cstring controlPlaneName;
        auto* typeArg = call->typeArguments->at(0);
        if (typeArg->is<IR::Type_StructLike>()) {
            auto structType = typeArg->to<IR::Type_StructLike>();
            controlPlaneName = structType->controlPlaneName();
        } else if (auto* typeName = typeArg->to<IR::Type_Name>()) {
            auto* referencedType = refMap->getDeclaration(typeName->path, true);
            CHECK_NULL(referencedType);
            controlPlaneName = referencedType->controlPlaneName();
        } else {
            static std::unordered_map<const IR::MethodCallExpression*, cstring> autoNames;
            auto it = autoNames.find(call);
            if (it == autoNames.end()) {
              controlPlaneName = "digest_" + cstring::to_cstring(autoNames.size());
              ::warning("Cannot find a good name for %1% method call, using "
                        "auto-generated name '%2%'", call, controlPlaneName);
              autoNames.emplace(call, controlPlaneName);
            } else {
              controlPlaneName = it->second;
            }
        }

        // Convert the generic type for the digest method call to a P4DataTypeSpec
        auto* typeSpec = TypeSpecConverter::convert(typeMap, refMap, typeArg, p4RtTypeInfo);
        BUG_CHECK(typeSpec != nullptr, "P4 type %1% could not be converted to P4Info P4DataTypeSpec");
        return Digest{controlPlaneName, typeSpec};
    }

    void addDigest(const P4RuntimeSymbolTableIface& symbols,
                   p4configv1::P4Info* p4Info,
                   const Digest& digest) {
        // Each call to digest() creates a new digest entry in the P4Info.
        // Right now we only take the type of data included in the digest
        // (encoded in its name) into account, but it may be that we should also
        // consider the receiver.
        auto id = symbols.getId(SymbolType::DIGEST(), digest.name);
        if (serializedInstances.find(id) != serializedInstances.end()) return;
        serializedInstances.insert(id);

        auto* digestInstance = p4Info->add_digests();
        digestInstance->mutable_preamble()->set_id(id);
        digestInstance->mutable_preamble()->set_name(digest.name);
        digestInstance->mutable_preamble()->set_alias(symbols.getAlias(digest.name));
        digestInstance->mutable_type_spec()->CopyFrom(*digest.typeSpec);
    }

    static boost::optional<ActionProfile>
    getActionProfile(cstring name,
                     const IR::Type_Extern* type,
                     const IR::Vector<IR::Argument>* arguments,
                     const IR::IAnnotated* annotations) {
        ActionProfileType actionProfileType;
        const IR::Expression* sizeExpression;
        if (type->name == P4V1::V1Model::instance.action_selector.name) {
            actionProfileType = ActionProfileType::INDIRECT_WITH_SELECTOR;
            sizeExpression = arguments->at(1)->expression;
        } else if (type->name == P4V1::V1Model::instance.action_profile.name) {
            actionProfileType = ActionProfileType::INDIRECT;
            sizeExpression = arguments->at(0)->expression;
        } else {
            return boost::none;
        }

        if (!sizeExpression->is<IR::Constant>()) {
            ::error("Action profile '%1%' has non-constant size '%1%'",
                    name, sizeExpression);
            return boost::none;
        }

        const int64_t size = sizeExpression->to<IR::Constant>()->asInt();
        return ActionProfile{name, actionProfileType, size, annotations};
    }

    /// @return the action profile referenced in @table's implementation property,
    /// if it has one, or boost::none otherwise.
    static boost::optional<ActionProfile>
    getActionProfile(const IR::P4Table* table, ReferenceMap* refMap, TypeMap* typeMap) {
        auto propertyName = P4V1::V1Model::instance.tableAttributes.tableImplementation.name;
        auto instance =
            getExternInstanceFromProperty(table, propertyName, refMap, typeMap);
        if (!instance) return boost::none;
        return getActionProfile(*instance->name, instance->type, instance->arguments,
                                getTableImplementationAnnotations(table, refMap));
    }

    /// @return the action profile declared with @decl
    static boost::optional<ActionProfile>
    getActionProfile(const IR::Declaration_Instance* decl, const IR::Type_Extern* type) {
        return getActionProfile(decl->controlPlaneName(), type, decl->arguments,
                                decl->to<IR::IAnnotated>());
    }

    /// @return true if @table's 'support_timeout' property exists and is true. This
    /// indicates that @table supports entry ageing.
    static bool getSupportsTimeout(const IR::P4Table* table) {
        auto timeout = table->properties->getProperty(P4V1::V1Model::instance
                                                      .tableAttributes
                                                      .supportTimeout.name);
        if (timeout == nullptr) return false;
        if (!timeout->value->is<IR::ExpressionValue>()) {
            ::error("Unexpected value %1% for supports_timeout on table %2%",
                    timeout, table);
            return false;
        }

        auto expr = timeout->value->to<IR::ExpressionValue>()->expression;
        if (!expr->is<IR::BoolLiteral>()) {
            ::error("Unexpected non-boolean value %1% for supports_timeout "
                    "property on table %2%", timeout, table);
            return false;
        }

        return expr->to<IR::BoolLiteral>()->value;
    }

    /// Set common fields between Counter and DirectCounter.
    template <typename Kind>
    void setCounterCommon(const P4RuntimeSymbolTableIface& symbols, Kind *counter,
                          const Helpers::Counterlike<CounterExtern>& counterInstance) {
        counter->mutable_preamble()->set_name(counterInstance.name);
        counter->mutable_preamble()->set_alias(symbols.getAlias(counterInstance.name));
        addAnnotations(counter->mutable_preamble(), counterInstance.annotations);
        auto counter_spec = counter->mutable_spec();

        if (counterInstance.unit == "packets") {
            counter_spec->set_unit(CounterSpec::PACKETS);
        } else if (counterInstance.unit == "bytes") {
            counter_spec->set_unit(CounterSpec::BYTES);
        } else if (counterInstance.unit == "packets_and_bytes") {
            counter_spec->set_unit(CounterSpec::BOTH);
        } else {
            counter_spec->set_unit(CounterSpec::UNSPECIFIED);
        }
    }

    void addCounter(const P4RuntimeSymbolTableIface& symbols,
                    p4configv1::P4Info* p4Info,
                    const Helpers::Counterlike<CounterExtern>& counterInstance) {
        if (counterInstance.table) {
            auto counter = p4Info->add_direct_counters();
            auto id = symbols.getId(SymbolType::DIRECT_COUNTER(),
                                    counterInstance.name);
            counter->mutable_preamble()->set_id(id);
            setCounterCommon(symbols, counter, counterInstance);
            auto tableId = symbols.getId(P4RuntimeSymbolType::TABLE(), *counterInstance.table);
            counter->set_direct_table_id(tableId);
        } else {
            auto counter = p4Info->add_counters();
            auto id = symbols.getId(SymbolType::COUNTER(),
                                    counterInstance.name);
            counter->mutable_preamble()->set_id(id);
            setCounterCommon(symbols, counter, counterInstance);
            counter->set_size(counterInstance.size);
        }
    }

    /// Set common fields between Meter and DirectMeter.
    template <typename Kind>
    void setMeterCommon(const P4RuntimeSymbolTableIface& symbols, Kind *meter,
                        const Helpers::Counterlike<MeterExtern>& meterInstance) {
        meter->mutable_preamble()->set_name(meterInstance.name);
        meter->mutable_preamble()->set_alias(symbols.getAlias(meterInstance.name));
        addAnnotations(meter->mutable_preamble(), meterInstance.annotations);
        auto meter_spec = meter->mutable_spec();
        meter_spec->set_type(MeterSpec::COLOR_UNAWARE);  // A default; this isn't exposed.

        if (meterInstance.unit == "packets") {
            meter_spec->set_unit(MeterSpec::PACKETS);
        } else if (meterInstance.unit == "bytes") {
            meter_spec->set_unit(MeterSpec::BYTES);
        } else {
            meter_spec->set_unit(MeterSpec::UNSPECIFIED);
        }
    }

    void addMeter(const P4RuntimeSymbolTableIface& symbols,
                  p4configv1::P4Info* p4Info,
                  const Helpers::Counterlike<MeterExtern>& meterInstance) {
        if (meterInstance.table) {
            auto meter = p4Info->add_direct_meters();
            auto id = symbols.getId(SymbolType::DIRECT_METER(),
                                    meterInstance.name);
            meter->mutable_preamble()->set_id(id);
            setMeterCommon(symbols, meter, meterInstance);
            auto tableId = symbols.getId(P4RuntimeSymbolType::TABLE(), *meterInstance.table);
            meter->set_direct_table_id(tableId);
        } else {
            auto meter = p4Info->add_meters();
            auto id = symbols.getId(SymbolType::METER(),
                                    meterInstance.name);
            meter->mutable_preamble()->set_id(id);
            setMeterCommon(symbols, meter, meterInstance);
            meter->set_size(meterInstance.size);
        }
    }

    void addRegister(const P4RuntimeSymbolTableIface& symbols,
                     p4configv1::P4Info* p4Info,
                     const Register& registerInstance) {
        auto register_ = p4Info->add_registers();
        auto id = symbols.getId(SymbolType::REGISTER(),
                                registerInstance.name);
        register_->mutable_preamble()->set_id(id);
        register_->mutable_preamble()->set_name(registerInstance.name);
        register_->mutable_preamble()->set_alias(symbols.getAlias(registerInstance.name));
        addAnnotations(register_->mutable_preamble(), registerInstance.annotations);
        register_->set_size(registerInstance.size);
        register_->mutable_type_spec()->CopyFrom(*registerInstance.typeSpec);
    }

    void addActionProfile(const P4RuntimeSymbolTableIface& symbols,
                          p4configv1::P4Info* p4Info,
                          const ActionProfile& actionProfile) {
        auto profile = p4Info->add_action_profiles();
        auto id = symbols.getId(SymbolType::ACTION_PROFILE(),
                                actionProfile.name);
        profile->mutable_preamble()->set_id(id);
        profile->mutable_preamble()->set_name(actionProfile.name);
        profile->mutable_preamble()->set_alias(symbols.getAlias(actionProfile.name));
        profile->set_with_selector(
            actionProfile.type == ActionProfileType::INDIRECT_WITH_SELECTOR);
        profile->set_size(actionProfile.size);

        auto tablesIt = actionProfilesRefs.find(actionProfile.name);
        if (tablesIt != actionProfilesRefs.end()) {
            for (const auto& table : tablesIt->second)
                profile->add_table_ids(symbols.getId(P4RuntimeSymbolType::TABLE(), table));
        }

        addAnnotations(profile->mutable_preamble(), actionProfile.annotations);
    }

 private:
    /// @return the table implementation property, or nullptr if the table has no
    /// such property.
    static const IR::Property* getTableImplementationProperty(const IR::P4Table* table) {
        return table->properties->getProperty(
            P4V1::V1Model::instance.tableAttributes.tableImplementation.name);
    }

    static const IR::IAnnotated* getTableImplementationAnnotations(
        const IR::P4Table* table, ReferenceMap* refMap) {
        auto impl = getTableImplementationProperty(table);
        if (impl == nullptr) return nullptr;
        if (!impl->value->is<IR::ExpressionValue>()) return nullptr;
        auto expr = impl->value->to<IR::ExpressionValue>()->expression;
        if (expr->is<IR::ConstructorCallExpression>()) return impl->to<IR::IAnnotated>();
        if (expr->is<IR::PathExpression>()) {
            auto decl = refMap->getDeclaration(expr->to<IR::PathExpression>()->path, true);
            return decl->to<IR::IAnnotated>();
        }
        return nullptr;
    }

    static boost::optional<cstring> getTableImplementationName(
        const IR::P4Table* table, ReferenceMap* refMap) {
        auto impl = getTableImplementationProperty(table);
        if (impl == nullptr) return boost::none;
        if (!impl->value->is<IR::ExpressionValue>()) {
            ::error("Expected implementation property value for table %1% to be an expression: %2%",
                    table->controlPlaneName(), impl);
            return boost::none;
        }
        auto expr = impl->value->to<IR::ExpressionValue>()->expression;
        if (expr->is<IR::ConstructorCallExpression>()) return impl->controlPlaneName();
        if (expr->is<IR::PathExpression>()) {
            auto decl = refMap->getDeclaration(expr->to<IR::PathExpression>()->path, true);
            return decl->controlPlaneName();
        }
        return boost::none;
    }

    ReferenceMap* refMap;
    TypeMap* typeMap;
    const IR::ToplevelBlock* evaluatedProgram;

    std::unordered_map<cstring, std::set<cstring> > actionProfilesRefs;

    /// The extern instances we've serialized so far. Used for deduplication.
    std::set<p4rt_id_t> serializedInstances;
};

P4RuntimeArchHandlerIface*
V1ModelArchHandlerBuilder::operator()(
    ReferenceMap* refMap, TypeMap* typeMap, const IR::ToplevelBlock* evaluatedProgram) const {
    return new P4RuntimeArchHandlerV1Model(refMap, typeMap, evaluatedProgram);
}

}  // namespace Standard

}  // namespace ControlPlaneAPI

/** @} */  /* end group control_plane */
}  // namespace P4
