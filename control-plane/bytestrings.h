/*
 * Copyright 2019 VMware, Inc.
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONTROL_PLANE_BYTESTRINGS_H_
#define CONTROL_PLANE_BYTESTRINGS_H_

#include <optional>
#include <string>

#include "lib/big_int.h"

namespace P4::IR {

class Type;
class Constant;
class BoolLiteral;
class Expression;

}  // namespace P4::IR

namespace P4 {

class TypeMap;

namespace ControlPlaneAPI {

/// getTypeWidth returns the width in bits for the @type, except if it is a
/// user-defined type with a @p4runtime_translation annotation, in which case it
/// returns W if the type is bit<W>, and 0 otherwise (i.e. if the type is
/// string).
int getTypeWidth(const IR::Type &type, const TypeMap &typeMap);

/// Convert a Constant to the P4Runtime bytes representation by calling
/// stringReprConstant.
std::optional<std::string> stringRepr(const IR::Constant *constant, int width);

/// Convert a BoolLiteral to the P4Runtime bytes representation by calling
/// stringReprConstant.
std::optional<std::string> stringRepr(const IR::BoolLiteral *constant, int width);

/// Convert an Expression to the P4Runtime bytes representation.
/// The type map is required to resolve complex expressions such as serializable enums.
std::optional<std::string> stringRepr(const TypeMap &typeMap, const IR::Expression *expression);

/// Convert a bignum to the P4Runtime bytes representation. The value must fit
/// within the provided @width expressed in bits. Padding will be added as
/// necessary (as the most significant bits).
std::optional<std::string> stringReprConstant(big_int value, int width);

}  // namespace ControlPlaneAPI

}  // namespace P4

#endif  // CONTROL_PLANE_BYTESTRINGS_H_
