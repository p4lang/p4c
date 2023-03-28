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

#ifndef CONTROL_PLANE_BYTESTRINGS_H_
#define CONTROL_PLANE_BYTESTRINGS_H_

#include <optional>
#include <string>

#include "lib/big_int_util.h"

namespace IR {

class Constant;
class BoolLiteral;

}  // namespace IR

namespace P4 {

namespace ControlPlaneAPI {

std::optional<std::string> stringRepr(const IR::Constant *constant, int width);

std::optional<std::string> stringRepr(const IR::BoolLiteral *constant, int width);

std::optional<std::string> stringReprConstant(big_int value, int width);

}  // namespace ControlPlaneAPI

}  // namespace P4

#endif  // CONTROL_PLANE_BYTESTRINGS_H_
