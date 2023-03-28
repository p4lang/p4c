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

#include "p4RuntimeArchStandard.h"

#include <optional>
#include <set>
#include <unordered_map>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "p4RuntimeArchHandler.h"
#include "typeSpecConverter.h"

namespace P4 {

/** \addtogroup control_plane
 *  @{
 */
namespace ControlPlaneAPI {

namespace Standard {

/// Implements @ref P4RuntimeArchHandlerIface for the v1model architecture. The
/// overridden methods will be called by the @P4RuntimeSerializer to collect and
/// serialize v1model-specific symbols which are exposed to the control-plane.
class P4RuntimeArchHandlerV1Model final : public P4RuntimeArchHandlerCommon<Arch::V1MODEL> {
 public:
    P4RuntimeArchHandlerV1Model(ReferenceMap *refMap, TypeMap *typeMap,
                                const IR::ToplevelBlock *evaluatedProgram)
        : P4RuntimeArchHandlerCommon<Arch::V1MODEL>(refMap, typeMap, evaluatedProgram) {}

    void collectExternFunction(P4RuntimeSymbolTableIface *symbols,
                               const P4::ExternFunction *externFunction) override {
        auto digest = getDigestCall(externFunction, refMap, typeMap, nullptr);
        if (digest) symbols->add(SymbolType::P4RT_DIGEST(), digest->name);
    }

    void addTableProperties(const P4RuntimeSymbolTableIface &symbols, p4configv1::P4Info *p4info,
                            p4configv1::Table *table, const IR::TableBlock *tableBlock) override {
        P4RuntimeArchHandlerCommon<Arch::V1MODEL>::addTableProperties(symbols, p4info, table,
                                                                      tableBlock);
        auto tableDeclaration = tableBlock->container;

        bool supportsTimeout = getSupportsTimeout(tableDeclaration);
        if (supportsTimeout) {
            table->set_idle_timeout_behavior(p4configv1::Table::NOTIFY_CONTROL);
        } else {
            table->set_idle_timeout_behavior(p4configv1::Table::NO_TIMEOUT);
        }
    }

    void addExternFunction(const P4RuntimeSymbolTableIface &symbols, p4configv1::P4Info *p4info,
                           const P4::ExternFunction *externFunction) override {
        auto p4RtTypeInfo = p4info->mutable_type_info();
        auto digest = getDigestCall(externFunction, refMap, typeMap, p4RtTypeInfo);
        if (digest) addDigest(symbols, p4info, *digest);
    }

    /// @return serialization information for the digest() call represented by
    /// @call, or std::nullopt if @call is not a digest() call or is invalid.
    static std::optional<Digest> getDigestCall(const P4::ExternFunction *function,
                                               ReferenceMap *refMap, P4::TypeMap *typeMap,
                                               p4configv1::P4TypeInfo *p4RtTypeInfo) {
        if (function->method->name != P4V1::V1Model::instance.digest_receiver.name)
            return std::nullopt;

        auto call = function->expr;
        BUG_CHECK(call->typeArguments->size() == 1, "%1%: Expected one type argument", call);
        BUG_CHECK(call->arguments->size() == 2, "%1%: Expected 2 arguments", call);

        // An invocation of digest() looks like this:
        //   digest<T>(receiver, { fields });
        // The name that shows up in the control plane API is the type name T. If T
        // doesn't have a name (e.g. tuple), we auto-generate one; ideally we would
        // be able to annotate the digest method call with a @name annotation in the
        // P4 but annotations are not supported on expressions.
        cstring controlPlaneName;
        auto *typeArg = call->typeArguments->at(0);
        if (typeArg->is<IR::Type_StructLike>()) {
            auto structType = typeArg->to<IR::Type_StructLike>();
            controlPlaneName = structType->controlPlaneName();
        } else if (auto *typeName = typeArg->to<IR::Type_Name>()) {
            auto *referencedType = refMap->getDeclaration(typeName->path, true);
            CHECK_NULL(referencedType);
            controlPlaneName = referencedType->controlPlaneName();
        } else {
            static std::unordered_map<const IR::MethodCallExpression *, cstring> autoNames;
            auto it = autoNames.find(call);
            if (it == autoNames.end()) {
                controlPlaneName = "digest_" + cstring::to_cstring(autoNames.size());
                ::warning(ErrorType::WARN_MISMATCH,
                          "Cannot find a good name for %1% method call, using "
                          "auto-generated name '%2%'",
                          call, controlPlaneName);
                autoNames.emplace(call, controlPlaneName);
            } else {
                controlPlaneName = it->second;
            }
        }

        // Convert the generic type for the digest method call to a P4DataTypeSpec
        auto *typeSpec = TypeSpecConverter::convert(refMap, typeMap, typeArg, p4RtTypeInfo);
        BUG_CHECK(typeSpec != nullptr,
                  "P4 type %1% could not "
                  "be converted to P4Info P4DataTypeSpec");
        return Digest{controlPlaneName, typeSpec, nullptr};
    }

    /// @return true if @table's 'support_timeout' property exists and is true. This
    /// indicates that @table supports entry ageing.
    static bool getSupportsTimeout(const IR::P4Table *table) {
        auto timeout = table->properties->getProperty(
            P4V1::V1Model::instance.tableAttributes.supportTimeout.name);
        if (timeout == nullptr) return false;
        if (!timeout->value->is<IR::ExpressionValue>()) {
            ::error(ErrorType::ERR_UNEXPECTED,
                    "Unexpected value %1% for supports_timeout on table %2%", timeout, table);
            return false;
        }

        auto expr = timeout->value->to<IR::ExpressionValue>()->expression;
        if (!expr->is<IR::BoolLiteral>()) {
            ::error(ErrorType::ERR_UNEXPECTED,
                    "Unexpected non-boolean value %1% for supports_timeout "
                    "property on table %2%",
                    timeout, table);
            return false;
        }

        return expr->to<IR::BoolLiteral>()->value;
    }
};

P4RuntimeArchHandlerIface *V1ModelArchHandlerBuilder::operator()(
    ReferenceMap *refMap, TypeMap *typeMap, const IR::ToplevelBlock *evaluatedProgram) const {
    return new P4RuntimeArchHandlerV1Model(refMap, typeMap, evaluatedProgram);
}

P4RuntimeArchHandlerIface *PSAArchHandlerBuilder::operator()(
    ReferenceMap *refMap, TypeMap *typeMap, const IR::ToplevelBlock *evaluatedProgram) const {
    return new P4RuntimeArchHandlerPSA(refMap, typeMap, evaluatedProgram);
}

P4RuntimeArchHandlerIface *PNAArchHandlerBuilder::operator()(
    ReferenceMap *refMap, TypeMap *typeMap, const IR::ToplevelBlock *evaluatedProgram) const {
    return new P4RuntimeArchHandlerPNA(refMap, typeMap, evaluatedProgram);
}

P4RuntimeArchHandlerIface *UBPFArchHandlerBuilder::operator()(
    ReferenceMap *refMap, TypeMap *typeMap, const IR::ToplevelBlock *evaluatedProgram) const {
    return new P4RuntimeArchHandlerUBPF(refMap, typeMap, evaluatedProgram);
}

}  // namespace Standard

}  // namespace ControlPlaneAPI

/** @} */ /* end group control_plane */
}  // namespace P4
