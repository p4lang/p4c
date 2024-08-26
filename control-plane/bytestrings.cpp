/*
Copyright 2019 VMware, Inc.

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

#include "bytestrings.h"

#include "control-plane/typeSpecConverter.h"
#include "frontends/p4/enumInstance.h"
#include "ir/ir.h"
#include "lib/algorithm.h"

namespace P4::ControlPlaneAPI {

int getTypeWidth(const IR::Type &type, const TypeMap &typeMap) {
    TranslationAnnotation annotation;
    if (hasTranslationAnnotation(&type, &annotation)) {
        // W if the type is bit<W>, and 0 if the type is string
        return annotation.controller_type.width;
    }
    /* Treat error type as string */
    if (type.is<IR::Type_Error>()) {
        return 0;
    }

    return typeMap.widthBits(&type, type.getNode(), false);
}

std::optional<std::string> stringReprConstant(big_int value, int width) {
    // TODO(antonin): support negative values
    if (value < 0) {
        ::P4::error(ErrorType::ERR_UNSUPPORTED, "%1%: Negative values not supported yet", value);
        return std::nullopt;
    }
    BUG_CHECK(width > 0, "Unexpected width 0");
    size_t bitsRequired = floor_log2(value) + 1;
    BUG_CHECK(static_cast<size_t>(width) >= bitsRequired, "Cannot represent %1% on %2% bits", value,
              width);
    // TODO(antonin): P4Runtime defines the canonical representation for bit<W>
    // value as the smallest binary string required to represent the value (no 0
    // padding). Unfortunately the reference P4Runtime implementation
    // (https://github.com/p4lang/PI) does not currently support the canonical
    // representation, so instead we use a padded binary string, which according
    // to the P4Runtime specification is also valid (but not the canonical
    // representation, which means no RW symmetry).
    // auto bytes = ROUNDUP(mpz_sizeinbase(value.get_mpz_t(), 2), 8);
    auto bytes = ROUNDUP(width, 8);
    std::vector<char> data(bytes);
    for (auto &d : data) {
        big_int v = (value >> (--bytes * 8)) & 0xff;
        d = static_cast<uint8_t>(v);
    }
    return std::string(data.begin(), data.end());
}

std::optional<std::string> stringRepr(const IR::Constant *constant, int width) {
    return stringReprConstant(constant->value, width);
}

std::optional<std::string> stringRepr(const IR::BoolLiteral *constant, int width) {
    auto v = static_cast<big_int>(constant->value ? 1 : 0);
    return stringReprConstant(v, width);
}

std::optional<std::string> stringRepr(const TypeMap &typeMap, const IR::Expression *expression) {
    int width = getTypeWidth(*expression->type, typeMap);
    // If the expression is a cast we keep its width, but use the casted expression for string
    // representation.
    while (const auto *cast = expression->to<IR::Cast>()) {
        expression = cast->expr;
    }
    auto *ei = EnumInstance::resolve(expression, &typeMap);
    if (expression->is<IR::Constant>()) {
        return stringRepr(expression->to<IR::Constant>(), width);
    }
    if (expression->is<IR::BoolLiteral>()) {
        return stringRepr(expression->to<IR::BoolLiteral>(), width);
    }
    if (ei != nullptr && ei->is<SerEnumInstance>()) {
        auto *sei = ei->to<SerEnumInstance>();
        return stringRepr(sei->value->to<IR::Constant>(), width);
    }
    ::P4::error(ErrorType::ERR_UNSUPPORTED, "%1% unsupported argument expression of type %2%",
                expression, expression->type->node_type_name());
    return std::nullopt;
}

}  // namespace P4::ControlPlaneAPI
