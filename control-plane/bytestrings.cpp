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

#include "ir/ir.h"

namespace P4 {

namespace ControlPlaneAPI {

/// Convert a bignum to the P4Runtime bytes representation. The value must fit
/// within the provided @width expressed in bits. Padding will be added as
/// necessary (as the most significant bits).
boost::optional<std::string> stringReprConstant(mpz_class value, int width) {
    // TODO(antonin): support negative values
    if (value < 0) {
        ::error("%1%: Negative values not supported yet", value);
        return boost::none;
    }
    BUG_CHECK(width > 0, "Unexpected width 0");
    auto bitsRequired = static_cast<size_t>(mpz_sizeinbase(value.get_mpz_t(), 2));
    BUG_CHECK(static_cast<size_t>(width) >= bitsRequired,
              "Cannot represent %1% on %2% bits", value, width);
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
    mpz_export(data.data(), NULL, 1 /* big endian word */, bytes,
               1 /* big endian bytes */, 0 /* full words */, value.get_mpz_t());
    return std::string(data.begin(), data.end());
}

/// Convert a Constant to the P4Runtime bytes representation by calling
/// stringReprConstant.
boost::optional<std::string> stringRepr(const IR::Constant* constant, int width) {
    return stringReprConstant(constant->value, width);
}

/// Convert a BoolLiteral to the P4Runtime bytes representation by calling
/// stringReprConstant.
boost::optional<std::string> stringRepr(const IR::BoolLiteral* constant, int width) {
    auto v = static_cast<mpz_class>(constant->value ? 1 : 0);
    return stringReprConstant(v, width);
}

}  // namespace ControlPlaneAPI

}  // namespace P4
