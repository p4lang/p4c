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

#ifndef _FRONTENDS_COMMON_CONSTANTPARSING_H_
#define _FRONTENDS_COMMON_CONSTANTPARSING_H_

#include "lib/cstring.h"

namespace IR {
class Constant;
}  // namespace IR

namespace Util {
class SourceInfo;
}  // namespace Util

/**
 * An unparsed numeric constant. We produce these as token values during
 * lexing. The parser is responsible for actually interpreting the raw text as a
 * numeric value and transforming it into an IR::Constant using parseConstant().
 *
 * To illustrate how a numeric constant is represented using this struct,
 * here is a breakdown of '16w0x12':
 *
 *          ___
 *         /                                    ___
 *         |                                   /
 *         |   bitwidth (if @hasWidth)        |       16
 *         |                                   \___
 *         |
 *         |                                    ___
 *         |                                   /
 *         |   separator (if @hasWidth)       |       w
 *         |                                   \___
 *  @text  |
 *         |                                    ___
 *         |                                   /
 *         |   ignored prefix of length @skip |       0x
 *         |                                   \___
 *         |
 *         |                                    ___
 *         |                                   /
 *         |   numeric value in base @base    |       w
 *         |                                   \___
 *         \___
 *
 * Simple numeric constants like '5' are specified by setting @hasWidth to
 * false and providing a @skip length of 0.
 */
struct UnparsedConstant {
    cstring text;   /// Raw P4 text which was recognized as a numeric constant.
    unsigned skip;  /// An ignored prefix of the numeric constant (e.g. '0x').
    unsigned base;  /// The base in which the constant is expressed.
    bool hasWidth;  /// If true, a bitwidth and separator are present.
};

std::ostream& operator<<(std::ostream& out, const UnparsedConstant& constant);

/**
 * Parses an UnparsedConstant @constant into an IR::Constant object, with
 * location information taken from @srcInfo. If parsing fails, an IR::Constant
 * containing the value @defaultValue is returned, and an error is reported.
 *
 * @return an IR::Constant parsed from @constant. If parsing fails, returns
 * either a default value.
 */
IR::Constant* parseConstant(const Util::SourceInfo& srcInfo,
                            const UnparsedConstant& constant,
                            long defaultValue);

/**
 * Parses a constant that should fit in an int value.
 * Reports an error if it does not.
 */
int parseConstantChecked(const Util::SourceInfo& srcInfo,
                         const UnparsedConstant& constant);

#endif /* _FRONTENDS_COMMON_CONSTANTPARSING_H_ */
