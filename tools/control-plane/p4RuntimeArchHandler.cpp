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

#include <boost/optional.hpp>

#include <sstream>  // for std::ostringstream

#include "frontends/common/resolveReferences/referenceMap.h"
// TODO(antonin): this include should go away when we cleanup getTableSize
// implementation.
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

#include "p4RuntimeArchHandler.h"

namespace P4 {

/** \addtogroup control_plane
 *  @{
 */
namespace ControlPlaneAPI {

namespace Helpers {

boost::optional<ExternInstance>
getExternInstanceFromProperty(const IR::P4Table* table,
                              const cstring& propertyName,
                              ReferenceMap* refMap,
                              TypeMap* typeMap,
                              bool *isConstructedInPlace) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return boost::none;
    if (!property->value->is<IR::ExpressionValue>()) {
        ::error("Expected %1% property value for table %2% to be an expression: %3%",
                propertyName, table->controlPlaneName(), property);
        return boost::none;
    }

    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    if (isConstructedInPlace) *isConstructedInPlace = expr->is<IR::ConstructorCallExpression>();
    if (expr->is<IR::ConstructorCallExpression>()
        && property->getAnnotation(IR::Annotation::nameAnnotation) == nullptr) {
        ::error("Table '%1%' has an anonymous table property '%2%' with no name annotation, "
                "which is not supported by P4Runtime", table->controlPlaneName(), propertyName);
        return boost::none;
    }
    auto name = property->controlPlaneName();
    auto externInstance = ExternInstance::resolve(expr, refMap, typeMap, name);
    if (!externInstance) {
        ::error("Expected %1% property value for table %2% to resolve to an "
                "extern instance: %3%", propertyName, table->controlPlaneName(),
                property);
        return boost::none;
    }

    return externInstance;
}

bool isExternPropertyConstructedInPlace(const IR::P4Table* table,
                                        const cstring& propertyName) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return false;
    if (!property->value->is<IR::ExpressionValue>()) {
        ::error("Expected %1% property value for table %2% to be an expression: %3%",
                propertyName, table->controlPlaneName(), property);
        return false;
    }

    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    return expr->is<IR::ConstructorCallExpression>();
}

int64_t getTableSize(const IR::P4Table* table) {
    // TODO(antonin): we should not be referring to v1model in this
    // architecture-independent code; each architecture may have a different
    // default table size.
    const int64_t defaultTableSize =
        P4V1::V1Model::instance.tableAttributes.defaultTableSize;

    auto sizeProperty = table->properties->getProperty("size");
    if (sizeProperty == nullptr) {
        return defaultTableSize;
    }

    if (!sizeProperty->value->is<IR::ExpressionValue>()) {
        ::error("Expected an expression for table size property: %1%", sizeProperty);
        return defaultTableSize;
    }

    auto expression = sizeProperty->value->to<IR::ExpressionValue>()->expression;
    if (!expression->is<IR::Constant>()) {
        ::error("Expected a constant for table size property: %1%", sizeProperty);
        return defaultTableSize;
    }

    const int64_t tableSize = expression->to<IR::Constant>()->asInt();
    return tableSize == 0 ? defaultTableSize : tableSize;
}

std::string serializeOneAnnotation(const IR::Annotation* annotation) {
    // we do not need custom serialization logic here: the P4Info should include
    // the annotation as it was in P4.
    std::ostringstream oss;
    ToP4 top4(&oss, false);
    annotation->apply(top4);
    auto serializedAnnnotation = oss.str();
    // remove the whitespace added by ToP4.
    serializedAnnnotation.pop_back();
    return serializedAnnnotation;
}

}  // namespace Helpers

}  // namespace ControlPlaneAPI

/** @} */  /* end group control_plane */
}  // namespace P4
