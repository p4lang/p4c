/*
Copyright 2018-present Barefoot Networks, Inc.

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

#ifndef DPDK_CONTROL_PLANE_P4RUNTIMEARCHDPDK_H_
#define DPDK_CONTROL_PLANE_P4RUNTIMEARCHDPDK_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

#include "control-plane/p4RuntimeArchHandler.h"
#include "control-plane/p4RuntimeArchStandard.h"
#include "p4/config/dpdk/p4info.pb.h"

using ::P4::ControlPlaneAPI::Helpers::getExternInstanceFromProperty;
using ::P4::ControlPlaneAPI::Helpers::setPreamble;

namespace p4configv1 = ::p4::config::v1;

namespace P4 {

/** \addtogroup control_plane
 *  @{
 */
namespace ControlPlaneAPI {

/// Declarations specific to standard architectures (v1model & PSA).
namespace Standard {

/// Extends @ref P4RuntimeSymbolType for the standard (v1model & PSA) extern
/// types.
class SymbolTypeDPDK : public P4RuntimeSymbolType {
 public:
    SymbolTypeDPDK() = delete;

    static P4RuntimeSymbolType ACTION_SELECTOR() {
        return P4RuntimeSymbolType::make(::dpdk::P4Ids::ACTION_SELECTOR);
    }
};

/// The information about an action profile which is necessary to generate its
/// serialized representation.
struct ActionSelector {
    const cstring name;  // The fully qualified external name of this action selector.
    const boost::optional<cstring> actionProfileName;  // If not known, we will generate
                         // an action profile instance.
    const int64_t size;  // TODO(hanw): size does not make sense with new ActionSelector P4 extern
    const int64_t maxGroupSize;
    const int64_t numGroups;
    const IR::IAnnotated* annotations;  // If non-null, any annotations applied to this action
                                        // profile declaration.
    bool selSuffix;

    p4rt_id_t getId(const P4RuntimeSymbolTableIface& symbols,
                                    const cstring blockPrefix = "") const {
        // auto symName = prefix(blockPrefix, name);
        auto symSelectorName = (selSuffix) ? name + "_sel" : name;
        if (actionProfileName) {
            return symbols.getId(SymbolTypeDPDK::ACTION_SELECTOR(), name);
        }
        return symbols.getId(SymbolTypeDPDK::ACTION_SELECTOR(), symSelectorName);
    }
};

/// Implements @ref P4RuntimeArchHandlerIface for the PSA architecture. The
/// overridden methods will be called by the @P4RuntimeSerializer to collect and
/// serialize PSA-specific symbols which are exposed to the control-plane.
class P4RuntimeArchHandlerPSAForDPDK final : public P4RuntimeArchHandlerCommon<Arch::PSA> {
 public:
    P4RuntimeArchHandlerPSAForDPDK(ReferenceMap* refMap,
                            TypeMap* typeMap,
                            const IR::ToplevelBlock* evaluatedProgram)
        : P4RuntimeArchHandlerCommon<Arch::PSA>(refMap, typeMap, evaluatedProgram) { }

    void collectTableProperties(P4RuntimeSymbolTableIface* symbols,
                                const IR::TableBlock* tableBlock) override {
        P4RuntimeArchHandlerCommon<Arch::PSA>::collectTableProperties(symbols, tableBlock);

        auto table = tableBlock->container;
        bool isConstructedInPlace = false;
        auto action_selector = getExternInstanceFromProperty(
            table, "ActionSelector",
            refMap, typeMap, &isConstructedInPlace);
        if (action_selector) {
            cstring tableName = *action_selector->name;
            if (action_selector->substitution.lookupByName("size")) {
                std::string selectorName(tableName + "_sel");
                symbols->add(SymbolTypeDPDK::ACTION_SELECTOR(), selectorName);
                symbols->add(SymbolType::ACTION_PROFILE(), tableName);
            } else {
                symbols->add(SymbolTypeDPDK::ACTION_SELECTOR(), tableName);
            }
        }
    }

    void collectExternInstance(P4RuntimeSymbolTableIface* symbols,
                               const IR::ExternBlock* externBlock) override {
        P4RuntimeArchHandlerCommon<Arch::PSA>::collectExternInstance(symbols, externBlock);

        auto decl = externBlock->node->to<IR::IDeclaration>();
        if (decl == nullptr) return;
        if (externBlock->type->name == "Digest") {
            symbols->add(SymbolType::DIGEST(), decl);
        }
    }

    void addTableProperties(const P4RuntimeSymbolTableIface& symbols,
                            p4configv1::P4Info* p4info,
                            p4configv1::Table* table,
                            const IR::TableBlock* tableBlock) override {
        P4RuntimeArchHandlerCommon<Arch::PSA>::addTableProperties(
            symbols, p4info, table, tableBlock);

        auto tableDeclaration = tableBlock->container;
        bool supportsTimeout = getSupportsTimeout(tableDeclaration);
        if (supportsTimeout) {
            table->set_idle_timeout_behavior(p4configv1::Table::NOTIFY_CONTROL);
        } else {
            table->set_idle_timeout_behavior(p4configv1::Table::NO_TIMEOUT);
        }
    }

    void addExternInstance(const P4RuntimeSymbolTableIface& symbols,
                           p4configv1::P4Info* p4info,
                           const IR::ExternBlock* externBlock) override {
        P4RuntimeArchHandlerCommon<Arch::PSA>::addExternInstance(
            symbols, p4info, externBlock);

        auto decl = externBlock->node->to<IR::Declaration_Instance>();
        if (decl == nullptr) return;
        auto p4RtTypeInfo = p4info->mutable_type_info();
        if (externBlock->type->name == "Digest") {
            auto digest = getDigest(decl, p4RtTypeInfo);
            if (digest) addDigest(symbols, p4info, *digest);
        }
    }

    /// @return serialization information for the Digest extern instacne @decl
    boost::optional<Digest> getDigest(const IR::Declaration_Instance* decl,
                                      p4configv1::P4TypeInfo* p4RtTypeInfo) {
        BUG_CHECK(decl->type->is<IR::Type_Specialized>(),
                  "%1%: expected Type_Specialized", decl->type);
        auto type = decl->type->to<IR::Type_Specialized>();
        BUG_CHECK(type->arguments->size() == 1, "%1%: expected one type argument", decl);
        auto typeArg = type->arguments->at(0);
        auto typeSpec = TypeSpecConverter::convert(refMap, typeMap, typeArg, p4RtTypeInfo);
        BUG_CHECK(typeSpec != nullptr,
                  "P4 type %1% could not be converted to P4Info P4DataTypeSpec");

        return Digest{decl->controlPlaneName(), typeSpec, decl->to<IR::IAnnotated>()};
    }

    /// @return true if @table's 'psa_idle_timeout' property exists and is true. This
    /// indicates that @table supports entry ageing.
    static bool getSupportsTimeout(const IR::P4Table* table) {
        auto timeout = table->properties->getProperty("psa_idle_timeout");

        if (timeout == nullptr) return false;

        if (auto exprValue = timeout->value->to<IR::ExpressionValue>()) {
            if (auto expr = exprValue->expression) {
                if (auto member = expr->to<IR::Member>()) {
                    if (member->member == "NOTIFY_CONTROL") {
                        return true;
                    } else if (member->member == "NO_TIMEOUT") {
                        return false;
                    }
                } else if (expr->is<IR::PathExpression>()) {
                    ::error(ErrorType::ERR_UNEXPECTED,
                        "Unresolved value %1% for psa_idle_timeout "
                        "property on table %2%. Must be a constant and one of "
                        "{ NOTIFY_CONTROL, NO_TIMEOUT }", timeout, table);
                    return false;
                }
            }
        }

        ::error(ErrorType::ERR_UNEXPECTED,
                "Unexpected value %1% for psa_idle_timeout "
                "property on table %2%. Supported values are "
                "{ NOTIFY_CONTROL, NO_TIMEOUT }", timeout, table);
        return false;
    }
};

/// The architecture handler builder implementation for PSA.
struct PSAArchHandlerBuilderForDPDK : public P4RuntimeArchHandlerBuilderIface {
    P4RuntimeArchHandlerIface* operator()(
        ReferenceMap* refMap,
        TypeMap* typeMap,
        const IR::ToplevelBlock* evaluatedProgram) const override {
    return new P4RuntimeArchHandlerPSAForDPDK(refMap, typeMap, evaluatedProgram);
}

};

}  // namespace Standard

}  // namespace ControlPlaneAPI

/** @} */  /* end group control_plane */
}  // namespace P4

#endif  /* DPDK_CONTROL_PLANE_P4RUNTIMEARCHDPDK_H_ */
