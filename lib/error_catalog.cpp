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

#include <map>
#include <string>
#include "error_catalog.h"
#include "error_reporter.h"

std::map<int, const std::string> ErrorReporter::errorCatalog = {
    { ErrorType::ERR_UNKNOWN,     "%1%: Unknown %2%"},      // args: %1% - IR node, %2% - what
    { ErrorType::ERR_UNSUPPORTED, "%1%: Unsupported %2%"},  // args: %1% - IR node, %2% - what
    { ErrorType::ERR_UNEXPECTED,  "%1%: Unexpected %2%"},   // args: %1% - IR node, %2% - what
    { ErrorType::ERR_EXPECTED,    "%1%: Expected %2%"},     // args: %1% - IR node, %2% - what
    { ErrorType::ERR_NOT_FOUND,   "%1%: %2% not found"},    // args: %1% - IR node, %2% - what
    { ErrorType::ERR_INVALID,     "%1%: Invalid %2%"},      // args: %1% - IR node, %2% - what
    { ErrorType::ERR_EXPRESSION,  "%1%: Expression %2%"},   // args: %1% - the expr, %2% - what
    { ErrorType::ERR_OVERLIMIT,   "%1%: Target supports %2% %3% %4%"}, // args: %1% - IR node, %2% - what, %3% - limit, %4% - unit
    { ErrorType::ERR_INSUFFICIENT,  "%1%: Target requires %2% %3% %4%"}, // args: %1% - IR node, %2% - limit, %3% - what, %4% - units
    { ErrorType::ERR_UNINITIALIZED, "%1%: Uninitialized %2%"}, // args: %1% - IR node, %2% - what
    // Warnings
    { ErrorType::WARN_UNKNOWN,     "%1%: Unknown %2%"},      // args: %1% - IR node, %2% - what
    { ErrorType::WARN_UNSUPPORTED, "%1%: Unsupported %2%"},  // args: %1% - IR node, %2% - what
    { ErrorType::WARN_MISMATCH,    "%1%: %2%"},              // args: %1% - IR node, $2% -what
};
