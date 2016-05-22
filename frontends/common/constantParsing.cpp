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

#include "ir/configuration.h"
#include "lib/gmputil.h"
#include "constantParsing.h"

IR::Constant* cvtCst(Util::SourceInfo srcInfo, const char* s, unsigned skip, unsigned base) {
    char *sep;
    auto size = strtol(s, &sep, 10);
    if (sep == nullptr || !*sep)
       BUG("Expected to find separator %1%", s);
    if (size <= 0) {
        ::error("%1%: Non-positive size %2%", srcInfo, size);
        return nullptr; }
    if (size > P4CConfiguration::MaximumWidthSupported) {
        ::error("%1%: %2% size too large", srcInfo, size);
        return nullptr; }

    bool isSigned = *sep == 's';
    mpz_class value = Util::cvtInt(sep+skip+1, base);
    const IR::Type* type = IR::Type_Bits::get(srcInfo, size, isSigned);
    IR::Constant* result = new IR::Constant(srcInfo, type, value, base);
    return result;
}
