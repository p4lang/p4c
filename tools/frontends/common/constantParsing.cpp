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

#include "constantParsing.h"

#include "ir/configuration.h"
#include "ir/ir.h"
#include "lib/gmputil.h"
#include "lib/source_file.h"

std::ostream& operator<<(std::ostream& out, const UnparsedConstant& constant) {
    out << "UnparsedConstant(" << constant.text << ','
                               << constant.skip << ','
                               << constant.base << ','
                               << constant.hasWidth << ')';
    return out;
}

/// A helper to parse constants which have an explicit width;
/// @see UnparsedConstant for an explanation of the parameters.
static IR::Constant*
parseConstantWithWidth(Util::SourceInfo srcInfo, const char* text,
                       unsigned skip, unsigned base) {
    char *sep;
    auto size = strtol(text, &sep, 10);
    sep += strspn(sep, " \t\r\n");
    if (sep == nullptr || !*sep)
       BUG("Expected to find separator %1%", text);
    if (size <= 0) {
        ::error("%1%: Non-positive size %2%", srcInfo, size);
        return nullptr; }
    if (size > P4CConfiguration::MaximumWidthSupported) {
        ::error("%1%: %2% size too large", srcInfo, size);
        return nullptr; }

    bool isSigned = *sep++ == 's';
    sep += strspn(sep, " \t\r\n");
    mpz_class value = Util::cvtInt(sep+skip, base);
    const IR::Type* type = IR::Type_Bits::get(srcInfo, size, isSigned);
    IR::Constant* result = new IR::Constant(srcInfo, type, value, base);
    return result;
}

IR::Constant* parseConstant(const Util::SourceInfo& srcInfo,
                            const UnparsedConstant& constant,
                            long defaultValue) {
    if (!constant.hasWidth) {
        auto value = Util::cvtInt(constant.text.c_str() + constant.skip, constant.base);
        return new IR::Constant(srcInfo, value, constant.base);
    }

    auto result = parseConstantWithWidth(srcInfo, constant.text.c_str(),
                                         constant.skip, constant.base);
    if (result == nullptr && defaultValue)
        return new IR::Constant(srcInfo, defaultValue);
    return result;
}
