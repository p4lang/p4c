/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_P4C_LIB_ERROR_TYPE_H_
#define BACKENDS_TOFINO_BF_P4C_LIB_ERROR_TYPE_H_

#include <lib/error_catalog.h>

namespace BFN {

using namespace P4;

/// Barefoot specific error and warning types.
class ErrorType : public P4::ErrorType {
 public:
    static const int WARN_TABLE_PLACEMENT = 1501;
    static const int WARN_PRAGMA_USE = 1502;
    static const int WARN_SUBSTITUTION = 1503;
    static const int WARN_PHV_ALLOCATION = 1504;
    static const int WARN_UNINIT_OVERLAY = 1505;

    static const int FIRST_BACKEND_WARNING = WARN_TABLE_PLACEMENT;

    /// in case we need to
    static ErrorType &getErrorTypes() {
        static ErrorType instance;
        return instance;
    }

    void printWarningsHelp(std::ostream &out);

 private:
    /// Barefoot specific error catalog.
    /// It simply adds all the supported errors and warnings in Barefoot's backend.
    ErrorType() {
        P4::ErrorCatalog::getCatalog()
            .add<ErrorMessage::MessageType::Warning, WARN_TABLE_PLACEMENT>("table-placement");
        P4::ErrorCatalog::getCatalog().add<ErrorMessage::MessageType::Warning, WARN_PRAGMA_USE>(
            "pragma-use");
        P4::ErrorCatalog::getCatalog().add<ErrorMessage::MessageType::Warning, WARN_SUBSTITUTION>(
            "substitution");
        P4::ErrorCatalog::getCatalog().add<ErrorMessage::MessageType::Warning, WARN_PHV_ALLOCATION>(
            "phv-allocation");
        P4::ErrorCatalog::getCatalog().add<ErrorMessage::MessageType::Warning, WARN_UNINIT_OVERLAY>(
            "uninitialized-overlay");
    }
};

}  // end namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_LIB_ERROR_TYPE_H_ */
